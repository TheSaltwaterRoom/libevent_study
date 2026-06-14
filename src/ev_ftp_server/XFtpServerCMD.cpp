#include "XFtpServerCMD.h"
#include <event2/bufferevent.h>
#include<iostream>
#include <string>
using namespace std;
void XFtpServerCMD::Reg(const std::string& cmd, XFtpTask *call)
{
	if (!call)
	{
		cout << "XFtpServerCMD::Reg call is null " << endl;
		return;
	}
	if (cmd.empty())
	{
		cout << "XFtpServerCMD::Reg cmd is null " << endl;
		return;
	}
	if (calls.find(cmd) != calls.end())
	{
		cout << cmd << " is alreay register" << endl;
		return;
	}
	calls[cmd] = call;
	calls_del[call] = 0;
}
void XFtpServerCMD::Read(struct bufferevent *bev)
{
	char data[1024] = { 0 };
	for (;;)
	{
		int len =static_cast<int>(bufferevent_read(bev, data, sizeof(data) - 1));
		if (len <= 0)
		{
			break;
		}
		recv_buffer.append(data, static_cast<size_t>(len));
	}

	for (;;)
	{
		size_t line_end = recv_buffer.find("\r\n");
		if (line_end == string::npos)
		{
			break;
		}

		string command = recv_buffer.substr(0, line_end + 2);
		recv_buffer.erase(0, line_end + 2);

		cout << "Recv CMD:" << command << flush;

		size_t type_end = command.find_first_of(" \r");
		string type = command.substr(0, type_end);
		cout << "type is [" << type << "]" << endl;

		if (calls.find(type) != calls.end())
		{
			XFtpTask *t = calls[type];
			t->cmdTask = this;
			t->ip = ip;
			t->port = port;
			t->base = base;
			t->Parse(type, command);
			if (type == "PORT")
			{
				ip = t->ip;
				port = t->port;
			}
		}
		else
		{
			string msg = "200 OK\r\n";
			bufferevent_write(bev, msg.c_str(), msg.size());
		}
	}
}
void XFtpServerCMD::Event(struct bufferevent *bev, short what)
{
	if (what & (BEV_EVENT_EOF | BEV_EVENT_ERROR | BEV_EVENT_TIMEOUT))
	{
		cout << "BEV_EVENT_EOF | BEV_EVENT_ERROR |BEV_EVENT_TIMEOUT" << endl;
		delete this;
	}
}
bool XFtpServerCMD::Init()
{
	cout << "XFtpServerCMD::Init()" << endl;
	// base socket
	bufferevent * bev = bufferevent_socket_new(base, sock, BEV_OPT_CLOSE_ON_FREE);
	if (!bev)
	{
		delete this;
		return false;
	}
	this->bev = bev;
	this->SetCallback(bev);

	timeval rt = {60,0};
	bufferevent_set_timeouts(bev, &rt, nullptr);
	string msg = "220 Welcome to libevent XFtpServer\r\n";
	bufferevent_write(bev, msg.c_str(), msg.size());
	return true;
}

XFtpServerCMD::~XFtpServerCMD()
{
	Close();
	for (auto & ptr : calls_del)
	{
		ptr.first->Close();
		delete ptr.first;
	}
}
