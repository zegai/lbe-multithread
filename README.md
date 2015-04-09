# lbe-multithread
a sample to use libevent support multi-thread



# MesQueue
作为一个全局消息队列,每个插入元素都会广播消息给工作线程,空闲的工作线程将会马上响应,同时动态插入和删除工作线程都非常方便.

# TWork
作为保存线程信息的结构体，包含了一个event_base结构体，线程本身将维持一个事件循环，当线程空闲时收到广播信息，将会试图获取
一个MesQueue节点即PNode。

#PNode
作为消息传递的内容，这部分自定义即可，如传入一个bufferevent指针，在工作线程中即可读出此事件中的数据进行处理.


