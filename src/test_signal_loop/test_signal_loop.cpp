#include <iostream>
#include <event2/event.h>
#include <signal.h>
using namespace std;
bool isexit = false;
static void Ctrl_C(evutil_socket_t sock, short which, void *arg)
{
	cout << "ctrl_c" << endl;

	event_base *base = (event_base *)arg;

	/*
		event_base_loopbreak(base)

		作用：
		让当前正在运行的 event_base_loop() / event_base_dispatch()
		在当前回调函数执行结束后尽快返回。

		特点：
		1. 不会中断当前 Ctrl_C 回调函数；
		2. 当前 Ctrl_C 回调执行完后，事件循环马上退出；
		3. 如果当前这一轮还有其他已经 active 的事件，通常不会继续执行；
		4. 适合“收到退出信号后立即退出事件循环”的场景。
	*/
	event_base_loopbreak(base);

	/*
		event_base_loopexit(base, NULL)

		作用：
		让事件循环退出。

		与 loopbreak 的区别：
		如果当前已经有一批 active 事件，
		loopexit(NULL) 会让这些 active 事件有机会执行完，
		然后事件循环再退出。

		所以它比 loopbreak 更“温和”。
	*/
	// event_base_loopexit(base, NULL);

	/*
		event_base_loopexit(base, &t)

		作用：
		延迟退出事件循环。

		例如下面表示：
		收到 Ctrl+C 后，不是马上退出，
		而是至少再运行约 3 秒，然后退出事件循环。

		在这 3 秒内，如果还有其他事件触发，
		libevent 仍然会继续处理这些事件。
	*/
	// timeval t = {3, 0};
	// event_base_loopexit(base, &t);
}

static void Kill(evutil_socket_t sock, short which, void *arg)
{
	cout << "Kill" << endl;

	event *ev = (event *)arg;
	// 如果处于非待决
	if (!evsignal_pending(ev, NULL))
	{
		event_del(ev);
		event_add(ev, NULL);
	}
}

int main(int argc, char **argv)
{
	event_base *base = event_base_new();

	// 添加ctrl+c信号事件，处于no pending
	// evsignal_new 隐藏的状态 EV_SIGNAL|EV_PERSIST
	event *csig = evsignal_new(base, SIGINT, Ctrl_C, base);
	if (!csig)
	{
		cerr << "SIGINT evsignal_new failed!" << endl;
		return -1;
	}

	// 添加事件到 pending
	if (event_add(csig, 0) != 0)
	{
		cerr << "SIGINT event_add failed!" << endl;
		return -1;
	}

	// 添加kill信号
	// 非持久事件，只进入一次 event_self_cbarg() 传递当前的event
	event *ksig = event_new(base, SIGTERM, EV_SIGNAL, Kill, event_self_cbarg());
	if (!ksig)
	{
		cerr << "SIGTERM event_new failed!" << endl;
		return -1;
	}

	// 添加事件到 pending
	if (event_add(ksig, 0) != 0)
	{
		cerr << "SIGTERM event_add failed!" << endl;
		return -1;
	}

	// 进入事件主循环
	event_base_dispatch(base);

	// EVLOOP_ONCE 阻塞直到有活动事件发生，然后在所有活动事件的回调函数都执行完毕后退出。
	// int ret = event_base_loop(base, EVLOOP_ONCE);
	// cout << "ret = " << ret << endl;

	// EVLOOP_NONBLOCK 不要阻塞：查看哪些事件已准备就绪，运行优先级最高的回调函数，然后退出。
	// while (!isexit)
	// {
	// 	event_base_loop(base, EVLOOP_NONBLOCK);
	// }

	// 不要因为没有待处理的事件而退出循环。相反，继续运行，直到 event_base_loopexit() 或 event_base_loopbreak() 导致停止。
	// event_base_loop(base, EVLOOP_NO_EXIT_ON_EMPTY);

	// | 写法                                               | 是否阻塞等待事件 | 没有事件时是否退出 |
	// | ------------------------------------------------ | -------: | --------: |
	// | `event_base_dispatch(base)`                      |        是 |         是 |
	// | `event_base_loop(base, 0)`                       |        是 |         是 |
	// | `event_base_loop(base, EVLOOP_NO_EXIT_ON_EMPTY)` |        是 |         否 |

	event_free(csig);
	event_base_free(base);

	return 0;
}
