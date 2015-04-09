#pragma once

#include <queue>
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
#include <io.h>
enum MSGTYPE
{
	TCP,
	RTSP,
	UDP,
	RTP,
};

typedef struct _QNode  
{
	MSGTYPE type;
	short events;
	void *arg;
}QNode,*PQNode;

typedef struct event LEvent;


#define MAX_WORK 4
class MesQueue
{
public:
	void MulticastInfo();

	void InsertQueue(PQNode InsNode);

	PQNode OutQueue();

	bool InsertWork(evutil_socket_t fd);

	bool DeleteWork(evutil_socket_t fd);

	static MesQueue* GetInstance();
protected:
	MesQueue();
	void MesLock();
	void MesUnLock();
	~MesQueue();
protected:
	CRITICAL_SECTION QueueLock;
	evutil_socket_t workfd[MAX_WORK];
	std::queue<PQNode> QList;
	unsigned char WorkCount;
};
typedef struct _WorkThread
{
	evutil_socket_t evpush[2];
	struct event_base	*base;
	struct event *evn;
	event_callback_fn callback;
	unsigned pid;
}TWork;


