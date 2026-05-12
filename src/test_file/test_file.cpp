#include <iostream>
#include <event2/event.h>

#ifndef _WIN32
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#endif

#include <thread>

using namespace std;

static void read_file(evutil_socket_t sock, short which, void *arg)
{
    char buf[1024] ={0};
    int len = read(sock,buf,sizeof(buf)-1);
    if(len>0){
        cout<<buf<<endl;
    }else{
        cout << "." << flush;
        this_thread::sleep_for(500ms);
    }
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

    event_config *conf = event_config_new();
    if (!conf)
    {
        std::cerr << "event_config_new failed" << std::endl;
        return 1;
    }

    // 设置支持文件描述符
    event_config_require_features(conf, EV_FEATURE_FDS);
    event_base *base = event_base_new_with_config(conf);
    event_config_free(conf);
    if (!base)
    {
        cerr << "event_base_new_with_config error" << endl;
        return -1;
    }

    // 打开文件只读，非阻塞
    int sock = open("/var/log/auth.log", O_RDONLY | O_NONBLOCK, 0);
    if (sock < 0)
    {
        cerr << "open failed" << endl;
        return -1;
    }

    //文件指针移动结尾处
    lseek(sock,0,SEEK_END);

    // 监听文件数据
    event *fev = event_new(base, sock, EV_READ | EV_PERSIST, read_file, 0);
    if (!fev)
    {
        cerr << "event_new failed!" << endl;
        return -1;
    }

    if (event_add(fev, 0) != 0)
    {
        cerr << "event_add failed!" << endl;
        return -1;
    }

    event_base_dispatch(base);
    event_base_free(base);
    event_free(fev);
    close(sock);

#if _WIN32
    WSACleanup();
#endif

    return 0;
}