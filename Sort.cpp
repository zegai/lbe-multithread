// Sort.cpp : 定义控制台应用程序的入口点。
// Edit zegai 2015/4/9
#pragma once
#include "stdafx.h"
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <signal.h>
#ifndef WIN32
#include <netinet/in.h>
# ifdef _XOPEN_SOURCE_EXTENDED
#  include <arpa/inet.h>
# endif
#include <sys/socket.h>
#endif

#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/listener.h>
#include <event2/util.h>
#include <event2/event.h>
#include <Windows.h>
#include <iostream>


#include "MesQueue.h"




static void Work(evutil_socket_t fd, short events, void *arg)
{
	char temp[100] = {0};
	recv(fd, temp, 100, 0);
	
	TWork *work = (TWork *)arg;
	printf("%s %d\n", temp, work->pid);

	PQNode node = MesQueue::GetInstance()->OutQueue();
	//while(node != NULL)
	//node = MesQueue::GetInstance()->OutQueue
	Sleep(100);
	if (node != NULL)
	{
		//队列的每个元素的生存周期应在使用完后由工作线程释放。
		free(node);
	}
	
}



static DWORD WINAPI FirstIn(LPVOID pvoid)
{
	
	TWork *work = (TWork *)pvoid;
	//开始此线程的事件循环
	event_base_dispatch(work->base);
	
	event_base_free(work->base);
	return 0;
}

int mes()
{
	struct event_base *base;
	struct evconnlistener *listener;
	struct event *signal_event;

	struct sockaddr_in sin;
#ifdef WIN32
	WSADATA wsa_data;
	WSAStartup(0x0202, &wsa_data);
#endif

	base = event_base_new();
	if (!base) {
		fprintf(stderr, "Could not initialize libevent!\n");
		return 1;
	}

	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(PORT);

	listener = evconnlistener_new_bind(base, listener_cb, (void *)base,
		LEV_OPT_REUSEABLE|LEV_OPT_CLOSE_ON_FREE, -1,
		(struct sockaddr*)&sin,
		sizeof(sin));

	if (!listener) {
		fprintf(stderr, "Could not create a listener!\n");
		return 1;
	}

	signal_event = evsignal_new(base, SIGINT, signal_cb, (void *)base);

	if (!signal_event || event_add(signal_event, NULL)<0) {
		fprintf(stderr, "Could not create/add a signal event!\n");
		return 1;
	}
	
	/*
	这个例子由Libevent自带的sample文件夹下的hello-world.c修改而成，只是作为libevent多线程的一个思路，代码虽然在windows下可使用，
	但Libevent2本身有对IOCP进行了封装，可以直接使用。当然直接不通过Libevent用IOCP应该说是最好的方法，IOCP自带消息队列，
	所以这个例子只作为在Linux下使用Libevent的参考代码，中间只需要替换线程创建函数和临界区的锁即可。
	*/
	
	/*
	TWork结构体中保存有工作线程的信息，每个线程将会有自己的event_base和事件循环，如何利用线程资源可以参考memcached,这里为一种解决方法。
	Libevent中通过事件激活回调函数，那么一个线程开始自己的事件循环后，每一次事件都会调用响应的回调函数，如EV_READ EV_WRITE，就如向event_base中
	添加一个EV_READ事件，当绑定的套接字有可读事件的时候就会触发事件。
	typedef struct _WorkThread
	{
		evutil_socket_t evpush[2];		//用于SOCKETPAIR,向其中可写套接字写入数据时，将触发对应线程
		struct event_base	*base;      //每一个线程拥有自己的event_base
		struct event *evn;						//用于绑定事件
		event_callback_fn callback;   //触发后的回调函数
		unsigned pid;									//自定义的线程编号
	}TWork;
	*/
	//多个线程信息变量
	TWork WorkThread[MAX_WORK];
	for (int z = 0; z < MAX_WORK; z++)
	{
		//Libevent实现了windows平台下的socketpair,前两参数只有AF_INET和SOCK_STREAM会被支持
		int ret = evutil_socketpair(AF_INET, SOCK_STREAM, 0, WorkThread[z].evpush);
		perror("ret");
		
		WorkThread[z].pid = z;
		//初始化一个新的event_base结构
		WorkThread[z].base = event_base_new();
		//设置触发回调函数
		WorkThread[z].callback = Work;
		//创建一个新的事件，指定这个触发这个事件的event_base，同时将可读的套接字作为事件触发的套接字，事件设置为EV_READ,EV_PERSISIT持续有效的，之后的参数好解释。
		WorkThread[z].evn = event_new(WorkThread[z].base, WorkThread->evpush[1], EV_READ | EV_PERSIST, Work, &WorkThread[z]);
		//event_base_set(WorkThread[z].base, WorkThread[z].evn);
		//添加这个事件，由于event_new中已经指定了event_base，所以直接添加事件。
		if (event_add(WorkThread[z].evn, 0) == -1)
		{
			perror("error insert");
		}
		//MesQueue相当于一个全局变量。
		//向自定义的消息队列插入工作线程，插入的内容只是该线程对应的可写套接字。如果插入的工作线程数量大于最大线程数将会返回错误，作为例子就不判断了。
		MesQueue::GetInstance()->InsertWork(WorkThread[z].evpush[0]);
		
	}
	//创建线程
	for (int i = 0; i < MAX_WORK; i++)
	{
		DWORD t = i; 
		::CreateThread(NULL, 0, FirstIn, &WorkThread[i], 0, &t);
	}
	//进入FirstIn函数


  //这里偷懒，不用外部触发来插入事件队列了，直接死循环插入，一种普通的生产者 消费者模型 。。。
	Sleep(1000);

	while(1)
	{
		//插入队列的最小单元，内容可自己定义
		PQNode newnode = new QNode;
		newnode->type = TCP;
		newnode->events = EV_READ;
		newnode->arg = NULL;
		//插入事件，这里每插入一个数据，[都将会对所有工作线程广播一个字节的数据作为唤醒信号]，如果线程正忙不会取队列中的数据，
		//只有完成任务的线程才会取获取队列元素。当然工作线程在唤醒处理一个元素之后，必须循环读取队列看是否有元素，
		//只有当队列的大小为0时才会将线程使用权交给原先的事件循环直到下一次唤醒。
		//
		MesQueue::GetInstance()->InsertQueue(newnode);
		Sleep(10);
	}


	/*struct event *timeout;
	struct timeval tv;

	timeout = evtimer_new(base, timeout_cb, NULL);
	evutil_timerclear(&tv);
	tv.tv_sec = 1;
	event_add(timeout, &tv);*/


	event_base_dispatch(base);

	evconnlistener_free(listener);
	event_free(signal_event);
	event_base_free(base);

	printf("done\n");
	return 0;
}







int _tmain(int argc, _TCHAR* argv[])
{
	mes();
	return 0;
}









static void
listener_cb(struct evconnlistener *listener, evutil_socket_t fd,
    struct sockaddr *sa, int socklen, void *user_data)
{
	struct event_base *base = (struct event_base *)user_data;
	struct bufferevent *bev;
	
	bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
	
	if (!bev) {
		fprintf(stderr, "Error constructing bufferevent!");
		event_base_loopbreak(base);
		return;
	}
	bufferevent_setcb(bev, conn_readcb, conn_writecb, conn_eventcb, NULL);

	bufferevent_enable(bev, EV_READ | EV_WRITE | EV_PERSIST);

	//bufferevent_write(bev, MESSAGE, strlen(MESSAGE));
	
}
static void
conn_readcb(struct bufferevent *bev, void *user_data)
{
	PQNode newnode = new QNode;
	newnode->type = TCP;
	newnode->events = EV_READ;
	newnode->arg = (void *)bev;

	MesQueue::GetInstance()->InsertQueue(newnode);

	/*memset(temp, 0, 4096);
	bufferevent_read(bev, temp, 4096);
	printf("%s\n",temp);
	
	st.append(temp);
	if(st.find_first_of("/r/n/r/n") != std::string::npos)
	{
		rtsp_string rs;
		rs.deal_requset(st);
		printf("%s\n",st.c_str());
		bufferevent_enable(bev, EV_WRITE);
		bufferevent_disable(bev, EV_READ);
		bufferevent_write(bev, st.c_str(), strlen(st.c_str()));
		bufferevent_write(bev, "\r\n", 2);
	}*/
	
		
}
static void
conn_writecb(struct bufferevent *bev, void *user_data)
{
	bufferevent_enable(bev, EV_READ);
	bufferevent_disable(bev, EV_WRITE);

}

static void
conn_eventcb(struct bufferevent *bev, short events, void *user_data)
{
	if (events & BEV_EVENT_EOF) {
		printf("Connection closed.\n");
	} else if (events & BEV_EVENT_ERROR) {
		printf("Got an error on the connection: %s\n",
		    strerror(errno));/*XXX win32*/
	}
	/* None of the other events can happen here, since we haven't enabled
	 * timeouts */
	bufferevent_free(bev);
}

static void
signal_cb(evutil_socket_t sig, short events, void *user_data)
{
	struct event_base *base = (struct event_base *)user_data;
	struct timeval delay = { 2, 0 };

	printf("Caught an interrupt signal; exiting cleanly in two seconds.\n");

	event_base_loopexit(base, &delay);
}
