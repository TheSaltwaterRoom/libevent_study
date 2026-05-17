#include <iostream>
#include <event2/event.h>
#include <event2/listener.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <string.h>

#ifndef _WIN32
#include <signal.h>
#endif

#define SPORT 5001

using namespace std;

void read_cb(struct bufferevent *bev, void *ctx)
{
    struct evbuffer *input = bufferevent_get_input(bev);
    size_t len = evbuffer_get_length(input);

    cout << "read_cb called, input len = " << len << endl;

    char buf[1024] = {0};
    int n = bufferevent_read(bev, buf, sizeof(buf));
    if (n > 0)
    {
        cout << "Received data: " << buf << endl;
    }

    if (strstr(buf, "quit") != nullptr)
    {
        cout << "Client requested to close the connection." << endl;
        // 关闭bufferevent对象，底层socket会被自动关闭
        bufferevent_free(bev);
        return;
    }

    // 发送数据 写入到输出缓冲区，底层socket可写时会自动发送
    const char *response = "okokokok\n";
    bufferevent_write(bev, response, strlen(response));
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

void listen_cb(struct evconnlistener *e, evutil_socket_t s, struct sockaddr *a, int socklen, void *arg)
{
    cout << "listen_cb called" << endl;

    event_base *base = (event_base *)arg;

    // 创建bufferevent对象，BEV_OPT_CLOSE_ON_FREE 清除bufferevent对象时关闭底层socket
    bufferevent *bev = bufferevent_socket_new(base, s, BEV_OPT_CLOSE_ON_FREE);

    // 添加监控事件
    bufferevent_enable(bev, EV_READ | EV_WRITE);

    // 设置水位
    // 读取水位
    bufferevent_setwatermark(bev, EV_READ,
                             5, // 低水位，默认0 0就是无限制 输入缓冲区可读数据达到5字节时触发read_cb回调
                             10 // 高水位，默认0 0就是无限制 输入缓冲区可读数据达到10字节时触发read_cb回调
    );

    // 写入水位
    bufferevent_setwatermark(bev, EV_WRITE,
                             5, // 低水位，默认0 0就是无限制 服务端自己的 output buffer 里面待发送的数据量降到 5 字节或以下时，触发 write_cb回调
                             0  // 高水位无效
    );

    // 设置超时
    struct timeval read_timeout = {3, 0}; // 读超时
    // struct timeval write_timeout = {20, 0}; // 写超时
    bufferevent_set_timeouts(bev, &read_timeout, 0);

    // 设置回调函数
    bufferevent_setcb(bev, read_cb, write_cb, event_cb, base);
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

    // 创建网络服务器
    sockaddr_in sin;
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(SPORT);
    evconnlistener *ev = evconnlistener_new_bind(
        base,                                      // libevent上下文
        listen_cb,                                 // 接收到连接的回调函数
        base,                                      // 回调函数获取的参数arg
        LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE, // evconnlistener关闭时同时关闭socket，地址重用
        10,                                        // listen backlog
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