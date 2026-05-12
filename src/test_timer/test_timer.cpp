#include <iostream>
#include <event2/event.h>

#ifndef _WIN32
#include <signal.h>
#endif

#include <thread>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <ctime>

using namespace std;
using namespace std::chrono_literals;

static struct timeval t1 = {1, 0};

/*
 * 用 steady_clock 记录程序启动时间。
 * steady_clock 是单调时钟，适合计算耗时。
 */
static auto g_start_time = std::chrono::steady_clock::now();

/*
 * 获取当前系统时间，格式类似：
 * 15:23:10.123
 */
static std::string current_time_string()
{
    using namespace std::chrono;

    auto now = system_clock::now();
    auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;

    std::time_t tt = system_clock::to_time_t(now);

    std::tm tm_buf;

#ifdef _WIN32
    localtime_s(&tm_buf, &tt);
#else
    localtime_r(&tt, &tm_buf);
#endif

    std::ostringstream oss;
    oss << std::put_time(&tm_buf, "%H:%M:%S")
        << "."
        << std::setw(3)
        << std::setfill('0')
        << ms.count();

    return oss.str();
}

/*
 * 获取程序启动后经过了多少秒。
 */
static double elapsed_seconds()
{
    using namespace std::chrono;

    auto now = steady_clock::now();
    auto diff = duration_cast<milliseconds>(now - g_start_time).count();

    return diff / 1000.0;
}

/*
 * 统一打印 timer 信息。
 */
static void print_timer(const char *name)
{
    cout << current_time_string()
         << "  +"
         << fixed << setprecision(3)
         << elapsed_seconds()
         << "s  "
         << name
         << endl;
}

static void time1(evutil_socket_t sock, short which, void *arg)
{
    print_timer("[time1]");

    event *ev = (event *)arg;

    // 如果处于非待决状态，就重新添加
    if (!evtimer_pending(ev, NULL)) {
        evtimer_add(ev, &t1);
    }
}

static void time2(evutil_socket_t sock, short which, void *arg)
{
    print_timer("[time2] begin");

    this_thread::sleep_for(3000ms);

    print_timer("[time2] end");
}

static void time3(evutil_socket_t sock, short which, void *arg)
{
    print_timer("[time3]");
}

int main(int argc, char **argv)
{
#if _WIN32
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
#else
    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
        std::cerr << "signal error" << std::endl;
        return 1;
    }
#endif

    event_base *base = event_base_new();
    if (!base) {
        cerr << "event_base_new failed!" << endl;
        return -1;
    }

    /*
     * ev1：非持久 timer，1 秒后触发。
     * 在 time1 回调里手动重新 add。
     */
    event *ev1 = evtimer_new(base, time1, event_self_cbarg());
    if (!ev1) {
        cerr << "evtimer_new ev1 failed!" << endl;
        event_base_free(base);
        return -1;
    }

    if (evtimer_add(ev1, &t1) != 0) {
        cerr << "evtimer_add ev1 failed!" << endl;
        event_free(ev1);
        event_base_free(base);
        return -1;
    }

    /*
     * ev2：持久 timer，1.2 秒触发一次。
     * 但是 time2 里面 sleep 3 秒，会阻塞整个 event loop。
     */
    struct timeval t2;
    t2.tv_sec = 1;
    t2.tv_usec = 200000;

    event *ev2 = event_new(base, -1, EV_PERSIST, time2, 0);
    if (!ev2) {
        cerr << "event_new ev2 failed!" << endl;
        event_free(ev1);
        event_base_free(base);
        return -1;
    }

    if (evtimer_add(ev2, &t2) != 0) {
        cerr << "evtimer_add ev2 failed!" << endl;
        event_free(ev2);
        event_free(ev1);
        event_base_free(base);
        return -1;
    }

    /*
     * ev3：持久 timer，3 秒触发一次。
     * 使用 common timeout 优化。
     */
    struct timeval tv_in = {3, 0};

    event *ev3 = event_new(base, -1, EV_PERSIST, time3, 0);
    if (!ev3) {
        cerr << "event_new ev3 failed!" << endl;
        event_free(ev2);
        event_free(ev1);
        event_base_free(base);
        return -1;
    }

    const timeval *t3 = event_base_init_common_timeout(base, &tv_in);

    if (evtimer_add(ev3, t3) != 0) {
        cerr << "evtimer_add ev3 failed!" << endl;
        event_free(ev3);
        event_free(ev2);
        event_free(ev1);
        event_base_free(base);
        return -1;
    }

    event_base_dispatch(base);

    event_free(ev3);
    event_free(ev2);
    event_free(ev1);
    event_base_free(base);

#if _WIN32
    WSACleanup();
#endif

    return 0;
}