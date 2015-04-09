// Sort.cpp : �������̨Ӧ�ó������ڵ㡣
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
		//���е�ÿ��Ԫ�ص���������Ӧ��ʹ������ɹ����߳��ͷš�
		free(node);
	}
	
}



static DWORD WINAPI FirstIn(LPVOID pvoid)
{
	
	TWork *work = (TWork *)pvoid;
	//��ʼ���̵߳��¼�ѭ��
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
	���������Libevent�Դ���sample�ļ����µ�hello-world.c�޸Ķ��ɣ�ֻ����Ϊlibevent���̵߳�һ��˼·��������Ȼ��windows�¿�ʹ�ã�
	��Libevent2�����ж�IOCP�����˷�װ������ֱ��ʹ�á���Ȼֱ�Ӳ�ͨ��Libevent��IOCPӦ��˵����õķ�����IOCP�Դ���Ϣ���У�
	�����������ֻ��Ϊ��Linux��ʹ��Libevent�Ĳο����룬�м�ֻ��Ҫ�滻�̴߳����������ٽ����������ɡ�
	*/
	
	/*
	TWork�ṹ���б����й����̵߳���Ϣ��ÿ���߳̽������Լ���event_base���¼�ѭ������������߳���Դ���Բο�memcached,����Ϊһ�ֽ��������
	Libevent��ͨ���¼�����ص���������ôһ���߳̿�ʼ�Լ����¼�ѭ����ÿһ���¼����������Ӧ�Ļص���������EV_READ EV_WRITE��������event_base��
	���һ��EV_READ�¼������󶨵��׽����пɶ��¼���ʱ��ͻᴥ���¼���
	typedef struct _WorkThread
	{
		evutil_socket_t evpush[2];		//����SOCKETPAIR,�����п�д�׽���д������ʱ����������Ӧ�߳�
		struct event_base	*base;      //ÿһ���߳�ӵ���Լ���event_base
		struct event *evn;						//���ڰ��¼�
		event_callback_fn callback;   //������Ļص�����
		unsigned pid;									//�Զ�����̱߳��
	}TWork;
	*/
	//����߳���Ϣ����
	TWork WorkThread[MAX_WORK];
	for (int z = 0; z < MAX_WORK; z++)
	{
		//Libeventʵ����windowsƽ̨�µ�socketpair,ǰ������ֻ��AF_INET��SOCK_STREAM�ᱻ֧��
		int ret = evutil_socketpair(AF_INET, SOCK_STREAM, 0, WorkThread[z].evpush);
		perror("ret");
		
		WorkThread[z].pid = z;
		//��ʼ��һ���µ�event_base�ṹ
		WorkThread[z].base = event_base_new();
		//���ô����ص�����
		WorkThread[z].callback = Work;
		//����һ���µ��¼���ָ�������������¼���event_base��ͬʱ���ɶ����׽�����Ϊ�¼��������׽��֣��¼�����ΪEV_READ,EV_PERSISIT������Ч�ģ�֮��Ĳ����ý��͡�
		WorkThread[z].evn = event_new(WorkThread[z].base, WorkThread->evpush[1], EV_READ | EV_PERSIST, Work, &WorkThread[z]);
		//event_base_set(WorkThread[z].base, WorkThread[z].evn);
		//�������¼�������event_new���Ѿ�ָ����event_base������ֱ������¼���
		if (event_add(WorkThread[z].evn, 0) == -1)
		{
			perror("error insert");
		}
		//MesQueue�൱��һ��ȫ�ֱ�����
		//���Զ������Ϣ���в��빤���̣߳����������ֻ�Ǹ��̶߳�Ӧ�Ŀ�д�׽��֡��������Ĺ����߳�������������߳������᷵�ش�����Ϊ���ӾͲ��ж��ˡ�
		MesQueue::GetInstance()->InsertWork(WorkThread[z].evpush[0]);
		
	}
	//�����߳�
	for (int i = 0; i < MAX_WORK; i++)
	{
		DWORD t = i; 
		::CreateThread(NULL, 0, FirstIn, &WorkThread[i], 0, &t);
	}
	//����FirstIn����


  //����͵���������ⲿ�����������¼������ˣ�ֱ����ѭ�����룬һ����ͨ�������� ������ģ�� ������
	Sleep(1000);

	while(1)
	{
		//������е���С��Ԫ�����ݿ��Լ�����
		PQNode newnode = new QNode;
		newnode->type = TCP;
		newnode->events = EV_READ;
		newnode->arg = NULL;
		//�����¼�������ÿ����һ�����ݣ�[����������й����̹߳㲥һ���ֽڵ�������Ϊ�����ź�]������߳���æ����ȡ�����е����ݣ�
		//ֻ�����������̲߳Ż�ȡ��ȡ����Ԫ�ء���Ȼ�����߳��ڻ��Ѵ���һ��Ԫ��֮�󣬱���ѭ����ȡ���п��Ƿ���Ԫ�أ�
		//ֻ�е����еĴ�СΪ0ʱ�ŻὫ�߳�ʹ��Ȩ����ԭ�ȵ��¼�ѭ��ֱ����һ�λ��ѡ�
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
