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
//#define FILEPATH "001.bmp"
#define FILEPATH "001.txt"
using namespace std;

struct ClientStatus {
	bool end = false;
	bool startSend = false;
	FILE* fp = nullptr;
	z_stream* z_output = nullptr;
	long readNum = 0;
	long sendNum = 0;
	~ClientStatus() {
		if (fp) {
			fclose(fp);
			fp = nullptr;
		}
		if (z_output) {
			deflateEnd(z_output);
			delete z_output;
			z_output = nullptr;
		}
	}
};

static enum bufferevent_filter_result filter_out(
	struct evbuffer* src, struct evbuffer* dst, ev_ssize_t dst_limit,
	enum bufferevent_flush_mode mode, void* ctx)
{
	//cout << "filter_out called, src len: " << evbuffer_get_length(src) << ", dst len: " << evbuffer_get_length(dst) << endl;

	//压缩文件
	ClientStatus* status = (ClientStatus*)ctx;

	if (!status->startSend) {
		char buf[1024] = { 0 };

		int n = evbuffer_remove(src, buf, sizeof(buf) - 1);

		if (n > 0)
		{
			evbuffer_add(dst, buf, n);

			return BEV_OK;
		}

		if (n == 0)
		{
			return BEV_NEED_MORE;
		}

		return BEV_OK;
	}

	//开始压缩文件
	//取出buffer中数据的引用
	evbuffer_iovec v_in[1];
	int n = evbuffer_peek(src, -1, 0, v_in, 1);
	if (n <= 0) {
		//调用writel回调，清理空间
		if (status->end) {
			return BEV_OK;
		}
		return BEV_NEED_MORE;//不会进入写入回调，继续等待数据进入缓冲区
	} 

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
	int re = deflate(z_output, Z_SYNC_FLUSH);
	if (re != Z_OK) {
		cout << "deflate error: " << re << endl;
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
	evbuffer_commit_space(dst, v_out,1);
	
	//cout << "client filter_out: read " << nread << " bytes, write " << nwrite << " bytes" << endl;
	status->readNum += nread;
	status->sendNum += nwrite;
	return BEV_OK;
}

static void client_read_cb(struct bufferevent* bev, void* ctx)
{
	struct evbuffer* input = bufferevent_get_input(bev);
	size_t len = evbuffer_get_length(input);

	//cout << "client read_cb called, input len = " << len << endl;
	ClientStatus* status = (ClientStatus*)ctx;
	// 002 接收服务端发送的OK回复
	char buf[1024] = { 0 };

	size_t n = bufferevent_read(bev, buf, sizeof(buf));
	if (n > 0)
	{
		if (strcmp(buf, "OK") == 0) {
			cout << "client Received data: ";
			cout.write(buf, n);
			cout << endl;
			status->startSend = true;
			//开始发送文件内容，触发写入回调
			bufferevent_trigger(bev, EV_WRITE, 0);

		}
		else {
			bufferevent_free(bev);
		}
	}
}

// write_cb 当输出缓冲区变成 0 字节时，触发
static void client_write_cb(struct bufferevent* bev, void* ctx)
{
	//cout << "write_cb called" << endl;
	ClientStatus* status = (ClientStatus*)ctx;

	if (!status || !status->fp) return;

	if (status->end) {
		/*
			因为用了 filter 后，输出方向有两层缓冲区
			filter bufferevent
			underlying bufferevent 以这个为准
		*/
		//判断缓冲是否有数据 ，如果有刷新
		//bev 是 filter bufferevent
		//be 是底层 socket bufferevent
		//evb 是底层 socket bufferevent 的输出缓冲区
		//len 是还有多少字节没真正写到 socket
		// 获取非过滤器buffer
		bufferevent* be = bufferevent_get_underlying(bev);
		//获取输出缓冲及其大小
		evbuffer* evb = bufferevent_get_output(be);
		int len = evbuffer_get_length(evb);
		if (len <= 0) {
			//立刻清理，如果缓冲有数据，不会发送
			cout << "client send total " << status->readNum << " bytes, write total " << status->sendNum << " bytes" << endl;
			bufferevent_free(bev);
			delete status;

			return;
		}

		//刷新缓冲区数据，立刻发送 催一下 filter，把能刷的数据继续往底层推。
		bufferevent_flush(bev, EV_WRITE, BEV_FLUSH);
		return;
	}

	FILE* fp = status->fp;
	char buf[1024] = { 0 };
	int len = fread(buf, 1, sizeof(buf), fp);
	if (len <= 0) {
		status->end = true;
		bufferevent_flush(bev, EV_WRITE, BEV_FLUSH);
		return;
	}

	//发送文件
	bufferevent_write(bev, buf, len);
}

static void client_event_cb(struct bufferevent* bev, short what, void* ctx)
{
	//cout << "client event_cb called, what: " << what << endl;

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
		// 001 发送文件名
		bufferevent_write(bev, FILEPATH, strlen(FILEPATH));

		//'fopen': This function or variable may be unsafe. Consider using fopen_s instead. To disable deprecation, use _CRT_SECURE_NO_WARNINGS. See online help for details.
		FILE* fp = fopen(FILEPATH, "rb");
		if (!fp) {
			cout << "open file " << FILEPATH << " failed!" << endl;
		}

		ClientStatus* status = new ClientStatus();
		status->fp = fp;

		//初始化zlib上下文，默认压缩
		status->z_output = new z_stream();
		deflateInit(status->z_output, Z_DEFAULT_COMPRESSION);

		//创建输出过滤
		bufferevent* bev_filter = bufferevent_filter_new(
			bev,
			0,
			filter_out,			   // 输出过滤函数
			BEV_OPT_CLOSE_ON_FREE | BEV_OPT_DEFER_CALLBACKS, // 关闭filter时同时关闭底层bufferevent
			0,
			status);
		
		// 设置bufferevent的回调函数 读取 事件（处理连接断开）
		bufferevent_setcb(bev_filter, client_read_cb, client_write_cb, client_event_cb,
			status // 回调函数获取的参数arg
		);
		// 添加监控事件
		bufferevent_enable(bev_filter, EV_READ | EV_WRITE);
	}
}

void Client(event_base* base) {
	//连接客户端
	sockaddr_in sin;
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(SPORT);
	evutil_inet_pton(AF_INET, "127.0.0.1", &sin.sin_addr.s_addr);
	bufferevent* bev = bufferevent_socket_new(base, -1, BEV_OPT_CLOSE_ON_FREE);

	//只绑定事件回调，连接成功事件在event_cb中处理
	bufferevent_setcb(bev, 0, 0, client_event_cb,
		0 // 回调函数获取的参数arg
	);
	// 添加监控事件
	bufferevent_enable(bev, EV_READ | EV_WRITE);

	bufferevent_socket_connect(bev, (sockaddr*)&sin, sizeof(sin));

	//发送文件名
	//接收回复确认OK
}