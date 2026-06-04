#include <event2/event.h>
#include <event2/listener.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <iostream>
#include <string.h>

#ifndef _WIN32
#include <signal.h>
#endif // !_WIN32

#define SPORT 5001

using namespace std;

void read_cb(struct bufferevent *bev, void *ctx)
{
	struct evbuffer *input = bufferevent_get_input(bev);
	size_t len = evbuffer_get_length(input);

	cout << "read_cb called, input len = " << len << endl;

	char buf[1024];

	size_t n = bufferevent_read(bev, buf, sizeof(buf));
	if (n > 0)
	{
		cout << "Received data: ";
		cout.write(buf, n);
		cout << endl;

		bufferevent_write(bev, "OK", 2);
	}
}

// write_cb 当输出缓冲区变成 0 字节时，触发
void write_cb(struct bufferevent *bev, void *ctx)
{
	cout << "write_cb called" << endl;
}

// event_cb 连接发生错误或者连接被对方关闭时 超时，触发
void event_cb(struct bufferevent *bev, short what, void *ctx)
{
	cout << "event_cb called, what: " << what << endl;

	// 读取超时事件发生后，数据读取停止
	if (what & BEV_EVENT_TIMEOUT && what & BEV_EVENT_READING)
	{
		cout << "Read timeout, closing connection." << endl;
		// bufferevent_enable(bev, EV_READ);
		bufferevent_free(bev);
		return;
	}

	// 打印其他事件
	if (what & BEV_EVENT_EOF)
	{
		cout << "Connection closed by peer." << endl;
	}
	if (what & BEV_EVENT_ERROR)
	{
		cout << "An error occurred on the connection." << endl;
		bufferevent_free(bev);
		return;
	}
	if (what & BEV_EVENT_CONNECTED)
	{
		cout << "Connection established." << endl;
	}
}

enum bufferevent_filter_result filter_in(
	struct evbuffer *src, struct evbuffer *dst, ev_ssize_t dst_limit,
	enum bufferevent_flush_mode mode, void *ctx)
{
	cout << "filter_in called, src len: " << evbuffer_get_length(src) << ", dst len: " << evbuffer_get_length(dst) << endl;

	char buf[1024] = {0};

	int n = evbuffer_remove(src, buf, sizeof(buf) - 1);

	if (n > 0)
	{
		cout << "filter data: " << buf << endl;
	
		evbuffer_add(dst, buf, n);

		return BEV_OK;
	}

	if (n == 0)
	{
		return BEV_NEED_MORE;
	}

	return BEV_ERROR;
}

enum bufferevent_filter_result filter_out(
	struct evbuffer *src, struct evbuffer *dst, ev_ssize_t dst_limit,
	enum bufferevent_flush_mode mode, void *ctx)
{
	cout << "filter_out called, src len: " << evbuffer_get_length(src) << ", dst len: " << evbuffer_get_length(dst) << endl;

	char buf[1024] = {0};

	int n = evbuffer_remove(src, buf, sizeof(buf) - 1);

	if (n > 0)
	{
		cout << "filter data: " << buf << endl;
		
		string recv = "";
		recv += "=================\n";
		recv.append(buf, n);
		recv += "=================\n";

		evbuffer_add(dst, recv.c_str(), recv.size());

		return BEV_OK;
	}

	if (n == 0)
	{
		return BEV_NEED_MORE;
	}

	return BEV_OK;
}

void listen_cb(struct evconnlistener *e, evutil_socket_t s, struct sockaddr *a, int socklen, void *arg)
{
	cout << "listen_cb called" << endl;
	event_base *base = (event_base *)arg;

	// 创建bufferevent对象，BEV_OPT_CLOSE_ON_FREE 清除bufferevent对象时关闭底层socket
	bufferevent *bev = bufferevent_socket_new(base, s, BEV_OPT_CLOSE_ON_FREE);

	bufferevent *bev_filter = bufferevent_filter_new(
		bev,
		filter_in,			   // 输入过滤函数
		filter_out,			   // 输出过滤函数
		BEV_OPT_CLOSE_ON_FREE, // 关闭filter时同时关闭底层bufferevent
		0,
		0);

	// 设置bufferevent的回调函数
	bufferevent_setcb(bev_filter, read_cb, write_cb, event_cb,
					  base // 回调函数获取的参数arg
	);
	// 添加监控事件
	bufferevent_enable(bev_filter, EV_READ | EV_WRITE);
}

int main()
{
#if _WIN32
	WSADATA wsa;
	WSAStartup(MAKEWORD(2, 2), &wsa);
#else
	if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)
	{
		std::cerr << "signal error" << std::endl;
		return 1;
	}
#endif // _WIN32

	event_base *base = event_base_new();
	if (base)
	{
		std::cout << "event_base_new success" << std::endl;
	}

	// 监听端口
	// socket,bind,listen,绑定事件
	sockaddr_in sin;
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(SPORT);
	evconnlistener *ev = evconnlistener_new_bind(
		base,									   // libevent上下文
		listen_cb,								   // 接收到连接的回调函数
		base,									   // 回调函数获取的参数arg
		LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE, // evconnlistener关闭时同时关闭socket，地址重用
		10,										   // listen backlog
		(sockaddr *)&sin,
		sizeof(sin));

	if (base)
		event_base_dispatch(base);
	if (ev)
		evconnlistener_free(ev);
	if (base)
		event_base_free(base);
#if _WIN32
	WSACleanup();
#endif
	return 0;
}