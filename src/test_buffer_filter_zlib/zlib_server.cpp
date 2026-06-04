#include <event2/event.h>
#include <event2/listener.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <iostream>
#include <string.h>
#include <zlib.h>

#ifndef _WIN32
#include <signal.h>
#endif // !_WIN32

#define SPORT 5001

using namespace std;

struct Status {
	bool start = false;
	FILE* fp = nullptr;
	z_stream* z_output = nullptr;
	long recvNum = 0;
	long writeNum = 0;
	~Status() {
		if (fp) {
			fclose(fp);
			fp = nullptr;
		}
		if (z_output) {
			inflateEnd(z_output);
			delete z_output;
			z_output = nullptr;
		}
	}
};

void read_cb(struct bufferevent* bev, void* ctx)
{
	struct evbuffer* input = bufferevent_get_input(bev);
	size_t len = evbuffer_get_length(input);

	//cout << "server read_cb called, input len = " << len << endl;

	Status* status = (Status*)ctx;
	if (!status->start)
	{
		char buf[1024] = {0};
		size_t n = bufferevent_read(bev, buf, sizeof(buf) - 1);
		if (n > 0)
		{
			cout << "server Received data: ";
			cout.write(buf, n);
			cout << endl;
			string out = "out/";
			out += buf;
			status->fp = fopen(out.c_str(), "wb");
			if (!status->fp) {
				cout << "open file " << out << " failed!" << endl;
				return;
			}

			// 回复用户消息，经过输出过滤器
			bufferevent_write(bev, "OK", 2);
			status->start = true;
			return;
		}
	}

	do {
		//写入文件
		char buf[1024] = { 0 };
		size_t n = bufferevent_read(bev, buf, sizeof(buf));
		if (n <= 0)break;
		fwrite(buf, 1, n, status->fp);
		fflush(status->fp);
	} while (evbuffer_get_length(bufferevent_get_input(bev)) > 0);
}

// event_cb 连接发生错误或者连接被对方关闭时 超时，触发
void event_cb(struct bufferevent* bev, short what, void* ctx)
{
	//cout << "server event_cb called, what: " << what << endl;

	Status* status = (Status*)ctx;
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
		cout << "server recv total " << status->recvNum << " bytes, write total " << status->writeNum << " bytes" << endl;
		delete status;
		bufferevent_free(bev);
	}
	if (what & BEV_EVENT_ERROR)
	{
		cout << "An error occurred on the connection." << endl;


		if (status->fp) {
			fclose(status->fp);
			status->fp = nullptr;
		}

		bufferevent_free(bev);

		return;
	}
	if (what & BEV_EVENT_CONNECTED)
	{
		cout << "Connection established." << endl;
	}
}

enum bufferevent_filter_result filter_in(
	struct evbuffer* src, struct evbuffer* dst, ev_ssize_t dst_limit,
	enum bufferevent_flush_mode mode, void* ctx)
{
	//cout << "filter_in called, src len: " << evbuffer_get_length(src) << ", dst len: " << evbuffer_get_length(dst) << endl;
	Status* status = (Status*)ctx;

	//接收文件名
	if (!status->start)
	{
		char buf[1024] = { 0 };

		int n = evbuffer_remove(src, buf, sizeof(buf) - 1);

		if (n > 0)
		{
			//cout << "filter data: " << buf << endl;

			evbuffer_add(dst, buf, n);

			return BEV_OK;
		}

		if (n == 0)
		{
			return BEV_NEED_MORE;
		}

		return BEV_ERROR;
	}

	//接收文件内容，解压缩
	evbuffer_iovec v_in[1];
	int n = evbuffer_peek(src, -1, 0, v_in, 1);
	if (n <= 0) return BEV_NEED_MORE;

	//zlib上下文
	z_stream* z_output = status->z_output;
	if (!z_output) return BEV_ERROR;
	//输入数据大小
	z_output->avail_in = v_in[0].iov_len;
	//输入数据地址
	z_output->next_in = (Bytef*)v_in[0].iov_base;

	//申请输出空间大小
	evbuffer_iovec v_out[1];
	evbuffer_reserve_space(dst, 4096, v_out, 1);

	//zlib输出空间大小
	z_output->avail_out = v_out[0].iov_len;
	//zlib输出空间地址
	z_output->next_out = (Bytef*)v_out[0].iov_base;

	//zlib压缩
	int re = inflate(z_output, Z_SYNC_FLUSH);
	if (re != Z_OK) {
		cout << "inflate error: " << re << endl;
		return BEV_ERROR;
	}

	//压缩后用了多少输入数据，从source evbuffer中移除
	//z_output->avail_in 未处理的数据大小
	int nread = v_in[0].iov_len - z_output->avail_in;

	//压缩后数据大小 传入des evbuffer
	// z_output->avail_out 还剩余的输出空间大小
	int nwrite = v_out[0].iov_len - z_output->avail_out;

	//移除source evbuffer中已处理的数据
	evbuffer_drain(src, nread);
	//传入des evbuffer已压缩数据的大小
	v_out[0].iov_len = nwrite;
	evbuffer_commit_space(dst, v_out, 1);

	//cout << "server filter_in: read " << nread << " bytes, write " << nwrite << " bytes" << endl;
	status->recvNum += nread;
	status->writeNum += nwrite;
	return BEV_OK;
}

void listen_cb(struct evconnlistener* e, evutil_socket_t s, struct sockaddr* a, int socklen, void* arg) {
	//cout << "listen_cb called" << endl;
	event_base* base = (event_base*)arg;

	// 创建bufferevent对象用来通信，BEV_OPT_CLOSE_ON_FREE 清除bufferevent对象时关闭底层socket
	bufferevent* bev = bufferevent_socket_new(base, s, BEV_OPT_CLOSE_ON_FREE);

	Status* status = new Status();
	status->z_output = new z_stream();
	inflateInit(status->z_output);
	//添加过滤 输入回调
	bufferevent* bev_filter = bufferevent_filter_new(
		bev,
		filter_in,			   // 输入过滤函数
		0,			   // 输出过滤函数
		BEV_OPT_CLOSE_ON_FREE, // 关闭filter时同时关闭底层bufferevent
		0,
		status);

	// 设置bufferevent的回调函数 读取 事件（处理连接断开）
	bufferevent_setcb(bev_filter, read_cb, 0, event_cb,
		status // 回调函数获取的参数arg
	);
	// 添加监控事件
	bufferevent_enable(bev_filter, EV_READ | EV_WRITE);
}

void Server(event_base* base) {
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
		10,//listen backlog
		(sockaddr*)&sin,
		sizeof(sin)
	);
}