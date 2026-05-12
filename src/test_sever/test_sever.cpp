#include <event2/event.h>
#include <event2/listener.h>
#include <iostream>
#include <string.h>

#ifndef _WIN32
#include <signal.h>
#endif // !_WIN32


#define SPORT 5001

using namespace std;
void listen_cb(struct evconnlistener* e, evutil_socket_t s, struct sockaddr* a, int socklen, void* arg) {
	cout << "listen_cb called" << endl;
}

int main()
{
#if _WIN32
    WSADATA wsa;
	WSAStartup(MAKEWORD(2, 2), &wsa);
#else
	if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
		std::cerr << "signal error" << std::endl;
		return 1;
	}
#endif // _WIN32

	event_base* base = event_base_new();
	if(base){
		std::cout << "event_base_new success" << std::endl;
	}

	//监听端口
	//socket,bind,listen,绑定事件
	sockaddr_in sin;
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(SPORT);
	evconnlistener* ev = evconnlistener_new_bind(
		base,//libevent上下文
		listen_cb,//接收到连接的回调函数
		base,//回调函数获取的参数arg
		LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE,//evconnlistener关闭时同时关闭socket，地址重用
		10,
		(sockaddr*)&sin,
		sizeof(sin)
		);

	if(base)
		event_base_dispatch(base);
	if(ev)
		evconnlistener_free(ev);
	if(base)
		event_base_free(base);
#if _WIN32
	WSACleanup();
#endif
	return 0;
}