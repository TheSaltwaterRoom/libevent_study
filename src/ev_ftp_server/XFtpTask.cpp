#include "XFtpTask.h"
#include <event2/event.h>
#include <event2/bufferevent.h>
#include <iostream>
#include <cstring>
using namespace std;
void XFtpTask::Send(const std::string& data)
{
	Send(data.c_str(),static_cast<int>(data.size()));
}
void XFtpTask::Send(const char*data, int datasize)
{
	if (!bev)
    {
	    return;
    }
	bufferevent_write(bev, data, datasize);
}
void XFtpTask::Close()
{
	CloseDataConnection();

	if (fp)
	{
		fclose(fp);
		fp = nullptr;
	}
}
bool XFtpTask::ConnectPORT()
{
    if (ip.empty() || port <= 0 || !base)
    {
        cout << "ConnectPORT failed: invalid ip, port or base" << endl;
        return false;
    }

    // 只关闭旧数据连接，不能关闭STOR/RETR打开的文件
    CloseDataConnection();

    bev = bufferevent_socket_new(
        base,
        -1,
        BEV_OPT_CLOSE_ON_FREE
    );

    if (!bev)
    {
        cout << "bufferevent_socket_new failed" << endl;
        return false;
    }

    sockaddr_in sin{};
    sin.sin_family = AF_INET;
    sin.sin_port = htons(port);

    if (evutil_inet_pton(
        AF_INET,
        ip.c_str(),
        &sin.sin_addr) != 1)
    {
        cout << "invalid ip: " << ip << endl;
        CloseDataConnection();
        return false;
    }

    SetCallback(bev);

    timeval rt = { 60, 0 };
    bufferevent_set_timeouts(bev, &rt, nullptr);

    if (bufferevent_socket_connect(
        bev,
        reinterpret_cast<sockaddr*>(&sin),
        sizeof(sin)) < 0)
    {
        cout << "bufferevent_socket_connect failed" << endl;
        CloseDataConnection();
        return false;
    }

    return true;
}
void XFtpTask::CloseDataConnection()
{
	if (bev)
	{
		bufferevent_free(bev);
		bev = nullptr;
	}
}
void XFtpTask::ResCMD(string msg) const
{
	if (!cmdTask || !cmdTask->bev)
    {
	    return;
    }
	cout << "ResCMD:" << msg << endl;
	if (msg[msg.size() - 1] != '\n')
    {
	    msg += "\r\n";
    }
	bufferevent_write(cmdTask->bev, msg.c_str(), msg.size());
}
void XFtpTask::SetCallback(struct bufferevent *bev)
{
	bufferevent_setcb(bev, ReadCB, WriteCB, EventCB, this);
	bufferevent_enable(bev, EV_READ | EV_WRITE);
}

void XFtpTask::ReadCB(bufferevent * bev, void *arg)
{
	auto *t = (XFtpTask *)arg;
	t->Read(bev);
}
void XFtpTask::WriteCB(bufferevent * bev, void *arg)
{
	auto *t = (XFtpTask *)arg;
	t->Write(bev);
}
void XFtpTask::EventCB(struct bufferevent *bev, short what, void *arg)
{
	auto *t = (XFtpTask *)arg;
	t->Event(bev,what);
}
