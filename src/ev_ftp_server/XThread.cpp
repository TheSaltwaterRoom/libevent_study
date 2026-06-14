#include "XThread.h"
#include <thread>
#include <iostream>
#include <event2/event.h>
#include "XTask.h"
#ifdef _WIN32

#else
#include <unistd.h>
#endif

using namespace std;

static void NotifyCB(evutil_socket_t fd, short which, void *arg)
{
	auto *t = (XThread *)arg;
	t->Notify(fd, which);
}
void XThread::Notify(evutil_socket_t fd, short which)
{
	char buf[2] = { 0 };
#ifdef _WIN32
	int re = recv(fd, buf, 1, 0);
#else
	//linux中是管道，不能用recv
	int re = read(fd, buf, 1);
#endif
	if (re <= 0)
    {
	    return;
    }
	cout << id << " thread " << buf << endl;
	XTask *task = nullptr;
	tasks_mutex.lock();
	if (tasks.empty())
	{
		tasks_mutex.unlock();
		return;
	}
	task = tasks.front();
	tasks.pop_front();
	tasks_mutex.unlock();
	task->Init();
}

void XThread::AddTask(XTask *t)
{
	if (!t)
    {
	    return;
    }
	t->base = this->base;
	tasks_mutex.lock();
	tasks.push_back(t);
	tasks_mutex.unlock();
}
void XThread::Activate() const
{

#ifdef _WIN32
	int re = send(this->notify_send_fd, "c", 1, 0);
#else
	int re = write(this->notify_send_fd, "c", 1);
#endif
	if (re <= 0)
	{
		cerr << "XThread::Activate() failed!" << endl;
	}
}

void XThread::Start()
{
	Setup();
	thread th(&XThread::Main,this);

	th.detach();
}

bool XThread::Setup()
{
	//windows用配对socket linux用管道
#ifdef _WIN32
	//创建一个socketpair可以互相通信, fds[0]读、fds[1]写
	evutil_socket_t fds[2];
	if (evutil_socketpair(AF_INET, SOCK_STREAM, 0, fds) < 0) {
		cerr << "socketpair failed: " << endl;
		return false;
	}

	//设置非阻塞
	evutil_make_socket_nonblocking(fds[0]);
	evutil_make_socket_nonblocking(fds[1]);
#else
	//创建的管理不能用send recv读取， 用read write读取
	int fds[2];

	if (pipe(fds)) {
		cerr << "pipe failed: " << endl;
		return false;
	}
#endif
	notify_send_fd = fds[1];

	event_config *ev_conf = event_config_new();
	event_config_set_flag(ev_conf, EVENT_BASE_FLAG_NOLOCK);
	this->base = event_base_new_with_config(ev_conf);
	event_config_free(ev_conf);
	if (!base)
	{
		cerr << "event_base_new_with_config failed in thread!" << endl;
		return false;
	}

	event *ev = event_new(base, fds[0], EV_READ | EV_PERSIST, NotifyCB, this);
	event_add(ev, nullptr);

	return true;
}
void XThread::Main()
{
	cout << id << " XThread::Main() begin" << endl;
	event_base_dispatch(base);
	event_base_free(base);

	cout << id << " XThread::Main() end" << endl;
}
