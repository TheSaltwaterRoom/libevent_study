#include <iostream>
#include <event2/event.h>
#include <signal.h>
using namespace std;

static void Ctrl_C(evutil_socket_t sock,short which,void *arg) {
	cout << "ctrl_c" << endl;
}

static void Kill(evutil_socket_t sock,short which,void *arg) {
	cout << "Kill" << endl;

	event *ev = (event *)arg;
	//如果处于非待决
	if(!evsignal_pending(ev,NULL)){
		event_del(ev);
		event_add(ev,NULL);
	}
}

int main(int argc, char** argv){
	event_base *base = event_base_new();

	//添加ctrl+c信号事件，处于no pending
	//evsignal_new 隐藏的状态 EV_SIGNAL|EV_PERSIST
	event *csig =  evsignal_new(base,SIGINT,Ctrl_C,base);
	if(!csig){
		cerr << "SIGINT evsignal_new failed!" << endl;
		return -1;
	}

	//添加事件到 pending
	if(event_add(csig,0) != 0){
		cerr << "SIGINT event_add failed!" << endl;
		return -1;
	}

	//添加kill信号
	//非持久事件，只进入一次 event_self_cbarg() 传递当前的event
	event *ksig =  event_new(base,SIGTERM,EV_SIGNAL,Kill,event_self_cbarg());
	if(event_add(csig,0) != 0){
		cerr << "SIGTERM event_new failed!" << endl;
		return -1;
	}

	//添加事件到 pending
	if(event_add(ksig,0) != 0){
		cerr << "SIGTERM event_add failed!" << endl;
		return -1;
	}

	//进入事件主循环
	event_base_dispatch(base);
	event_free(csig);
	event_base_free(base);

	
	return 0;
}
