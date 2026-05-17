#include <iostream>
#include <event2/event.h>

#ifndef _WIN32
#include <signal.h>
#endif

#include <string.h>

using namespace std;

#define SPORT 5001

// 正常断开连接，超时，会调用
void client_cb(evutil_socket_t s, short w, void *arg)
{
    // 水平触发LT 只有有数据没有处理，会一直进入
    // 边缘触发有数据时只进入一次
    //  cout<<"."<<endl;return;
    event *ev = (event *)arg;

    if (w & EV_TIMEOUT)
    {
        cout << "client timeout" << endl;

        event_free(ev);
        evutil_closesocket(s);
        return;
    }

    char buf[1024] = {0};
    // char buf[3] = {0};
    int len = recv(s, buf, sizeof(buf) - 1, 0);

    if (len > 0)
    {
        cout << buf << endl;
        send(s, "ok", 2, 0);
    }
    else
    {
        // 需要清理 event
        cout << "event_free" << flush;

        event_free(ev);

        evutil_closesocket(s);
    }
}

static void listen_cb(evutil_socket_t sock, short which, void *arg)
{
    cout << "listen_cb" << endl;

    sockaddr_in sin;
    memset(&sin, 0, sizeof(sin));

    socklen_t size = sizeof(sin);

    // 接收客户端连接
    evutil_socket_t client = accept(sock, (sockaddr *)&sin, &size);
    if (client < 0)
    {
        cerr << "accept error: " << strerror(errno) << endl;
        return;
    }

    char ip[INET_ADDRSTRLEN] = {0};

    evutil_inet_ntop(AF_INET, &sin.sin_addr, ip, sizeof(ip));

    cout << "client ip is " << ip << endl;
    cout << "client port is " << ntohs(sin.sin_port) << endl;

    // 客户端数据读取事件
    event_base *base = (event_base *)arg;
    //水平触发LT
    event *ev = event_new(
        base,
        client,
        EV_READ | EV_PERSIST,
        client_cb,
        event_self_cbarg());

    //边缘触发ET
    // event *ev = event_new(
    //     base,
    //     client,
    //     EV_READ | EV_PERSIST | EV_ET,
    //     client_cb,
    //     event_self_cbarg());

    timeval t = {10, 0};

    event_add(ev, &t);
}

int main(int argc, char **argv)
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
#endif

    event_base *base = event_base_new();
    if (!base)
    {
        cerr << "event_base_new failed!" << endl;
        return -1;
    }

    evutil_socket_t sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        cerr << "socket error: " << strerror(errno) << endl;
        return -1;
    }

    // 设置地址复用和非阻塞
    evutil_make_socket_nonblocking(sock);
    evutil_make_listen_socket_reuseable(sock);

    sockaddr_in sin;
    memset(&sin, 0, sizeof(sin));

    sin.sin_family = AF_INET;
    sin.sin_port = htons(SPORT);

    int re = ::bind(sock, (sockaddr *)&sin, sizeof(sin));
    if (re != 0)
    {
        cerr << "bind error: " << strerror(errno) << endl;
        return -1;
    }

    // 开始监听
    listen(sock, 10);

    // 开始接受连接事件 默认水平触发
    event *ev = event_new(base, sock, EV_READ | EV_PERSIST, listen_cb, base);
    event_add(ev, 0);

    event_base_dispatch(base);
    event_base_free(base);
    evutil_closesocket(sock);

#if _WIN32
    WSACleanup();
#endif

    return 0;
}