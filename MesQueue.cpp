#include "stdafx.h"
#include "MesQueue.h"


void MesQueue::MulticastInfo()
{
	for (int i = 0; i < MAX_WORK; i++)
	{
		if (workfd[i] != 0)
		{
			send(workfd[i], "o", sizeof("o") - 1, NULL);
		}
	}
}

void MesQueue::InsertQueue(PQNode InsNode)
{
	assert(InsNode);
	MesLock();
	QList.push(InsNode);
	//printf("Qlist Ins:%d\n", QList.size());
	MesUnLock();
	MulticastInfo();
}

PQNode MesQueue::OutQueue()
{
	PQNode node = NULL;
	MesLock();
	if (QList.size())
	{
		PQNode node = QList.front();
		QList.pop();
		//printf("Qlist Out:%d\n", QList.size());
	}
	MesUnLock();
	return node;
}

bool MesQueue::InsertWork(evutil_socket_t fd)
{
	bool flag = false;
	MesLock();
	if (fd > 0 && WorkCount < MAX_WORK)
	{
		for (int i = 0; i < MAX_WORK ; i++)
		{
			if (workfd[i] == 0)
			{
				workfd[i] = fd;
				WorkCount ++;
				flag = true;
				break;
			}
		}
	}
	MesUnLock();
	return flag;
}

bool MesQueue::DeleteWork(evutil_socket_t fd)
{
	bool flag = false;
	MesLock();
	for (int i = 0; i < MAX_WORK ; i++)
	{
		if (workfd[i] == fd)
		{
			workfd[i] = 0;
			WorkCount --;
			flag = true;
		}
	}
	MesUnLock();
	return flag;
}

MesQueue* MesQueue::GetInstance()
{
	static MesQueue* que = NULL;
	if (que == NULL)
	{
		que = new MesQueue;
		return que;
	}
	return que;
}


MesQueue::MesQueue(){
	InitializeCriticalSection(&QueueLock);
	WorkCount = 0;
	memset(workfd, 0, sizeof(workfd));
}
void MesQueue::MesLock(){EnterCriticalSection(&QueueLock);}
void MesQueue::MesUnLock(){LeaveCriticalSection(&QueueLock);}
MesQueue::~MesQueue()
{
	MesLock();
	while(QList.size())
	{
		delete QList.front();
		QList.pop();
	}
	MesUnLock();
	DeleteCriticalSection(&QueueLock);
}