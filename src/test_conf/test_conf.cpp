#include <event2/event.h>
#include <event2/listener.h>
#include <event2/thread.h>
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
    //你往一个“读端已经关闭”的管道或 socket 里写数据。Linux 可能给服务端进程发送 SIGPIPE SIGPIPE 的默认动作是：终止当前进程
	if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
		std::cerr << "signal error" << std::endl;
		return 1;
	}
#endif // _WIN32

	//创建配置上下文
	event_config* conf = event_config_new();
	if (!conf) {
		std::cerr << "event_config_new failed" << std::endl;
		return 1;
	}

	const char**methos =  event_get_supported_methods();
	cout << "supported methods: " << endl;
	for (size_t i = 0; methos[i] != NULL; i++)
	{
		cout << methos[i] << endl;
	}

	//设置特征
	//按 libevent-2.1.12-stable 来看：
	//平台 / 后端		ET		O(1)	FDS		EARLY_CLOSE
	//Linux epoll	支持		支持		不声明	支持
	//Linux poll	不支持	不支持	支持		视 POLLRDHUP 而定
	//Linux select	不支持	不支持	支持		不支持
	//Windows win32	不支持	不支持	不支持	不支持
	// 
	//linux设置了EV_FEATURE_FDS feature 组合必须被同一个后端同时支持；如果没有后端同时支持，就创建失败，在windows中EV_FEATURE_FDS无效
	//windows中不支持EV_FEATURE_ET
	//event_config_require_features(conf, EV_FEATURE_ET| EV_FEATURE_FDS);//失败的，没有后端支持
	//event_config_require_features(conf, EV_FEATURE_FDS);//不支持epoll,一旦设置就变成poll,linux默认网络模型是epoll

	//设置网络模型 select
	//event_config_avoid_method(conf, "epoll");
	//event_config_avoid_method(conf, "poll");

	//windows支持IOCP（线程池）
#ifdef _WIN32
	event_config_set_flag(conf, EVENT_BASE_FLAG_STARTUP_IOCP);
	//初始化iocp的线程
	evthread_use_windows_threads();
	//设置cpu数量
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	event_config_set_num_cpus_hint(conf, si.dwNumberOfProcessors);
#endif // _WIN32

	//初始化libevent上下文
	event_base* base = event_base_new_with_config(conf);
	event_config_free(conf);
	if (!base) {
		cerr << "event_base_new_with_config error" << endl;
		base = event_base_new();
		if (!base) {
			cerr << "event_base_new error" << endl;
			return 1;
		}
	}
	else {
		//获取当前网络模型
		cout << "current method: " << event_base_get_method(base) << endl;

		//确认特征是否生效
		int f = event_base_get_features(base);
		if (f & EV_FEATURE_ET) {
			cout << "EV_FEATURE_ET is supported" << endl;
		}
		else {
			cout << "EV_FEATURE_ET is not supported" << endl;
		}

		if (f & EV_FEATURE_O1) {
			cout << "EV_FEATURE_O1 is supported" << endl;
		}
		else {
			cout << "EV_FEATURE_O1 is not supported" << endl;
		}

		if (f & EV_FEATURE_FDS) {
			cout << "EV_FEATURE_FDS is supported" << endl;
		}
		else {
			cout << "EV_FEATURE_FDS is not supported" << endl;
		}

		if (f & EV_FEATURE_EARLY_CLOSE) {
			cout << "EV_FEATURE_EARLY_CLOSE is supported" << endl;
		}
		else {
			cout << "EV_FEATURE_EARLY_CLOSE is not supported" << endl;
		}

		cout << "event_base_new_with_config success" << endl;

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

		if (!ev) {
			cerr << "evconnlistener_new_bind failed: "
				<< evutil_socket_error_to_string(EVUTIL_SOCKET_ERROR())
				<< endl;
		}

		if (base)
			event_base_dispatch(base);
		if (ev)
			evconnlistener_free(ev);
		if (base)
			event_base_free(base);
	}
	
	
#if _WIN32
	WSACleanup();
#endif
	return 0;
}