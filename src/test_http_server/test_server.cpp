#include <event2/event.h>
#include <event2/listener.h>
#include <event2/http.h>
#include <event2/keyvalq_struct.h>
#include <event2/buffer.h>
#include <iostream>
#include <string.h>

#ifndef _WIN32
#include <signal.h>
#endif // !_WIN32
#define WEBROOT "."
#define DEFAULTINDEX "index.html"

using namespace std;

void http_cb(struct evhttp_request* request, void* arg) {
	cout << "HTTP request received" << endl;
	//1 获取浏览器的请求信息
	const char* uri = evhttp_request_get_uri(request);
	cout << "URI: " << uri << endl;
	// 请求类型 GET
	string cmdtype;
	switch (evhttp_request_get_command(request)) {
	case EVHTTP_REQ_GET:
		cmdtype = "GET";
		break;
	case EVHTTP_REQ_POST:
		cmdtype = "POST";
		break;
	default:
		cmdtype = "UNKNOWN";
	}
	cout << "Command: " << cmdtype << endl;

	// 分析出请求的文件uri
	// 设置根目录
	string filepath = WEBROOT;
	filepath += uri;
	if (strcmp(uri, "/") == 0) {
		filepath += DEFAULTINDEX;
	}

	//消息报头

	//是支持图片 js css 下载zip文件
	evkeyvalq* outheaders = evhttp_request_get_output_headers(request);

	int pos = filepath.rfind('.');
	string postfix = filepath.substr(pos + 1,filepath.size() - (pos + 1));
	if (postfix == "html" || postfix == "htm") {
		evhttp_add_header(outheaders, "Content-Type", "text/html;charset=utf-8");
	}
	else if (postfix == "jpg" || postfix == "gif" || postfix == "png") {
		string tmp = "image/" + postfix;
		evhttp_add_header(outheaders, "Content-Type", tmp.c_str());
	}
	else if (postfix == "js") {
		evhttp_add_header(outheaders, "Content-Type", "application/javascript");
	}
	else if (postfix == "css") {
		evhttp_add_header(outheaders, "Content-Type", "text/css");
	}
	else if (postfix == "zip") {
		evhttp_add_header(outheaders, "Content-Type", "application/zip");
	}

	evkeyvalq* headers = evhttp_request_get_input_headers(request);
	cout << "=======headers========\n" << endl;
	for (evkeyval* p = headers->tqh_first; p != NULL; p = p->next.tqe_next) {
		cout << p->key << ": " << p->value << endl;
	}

	evbuffer* inbuf = evhttp_request_get_input_buffer(request);
	char buf[1024] = { 0 };
	cout << "=======body========" << endl;
	while (evbuffer_get_length(inbuf) > 0) {
		int n = evbuffer_remove(inbuf, buf, sizeof(buf) - 1);
		if (n <= 0) {
			break;
		}
		buf[n] = '\0';
		cout << buf << endl;
	}

	//2 回复浏览器
	// 状态行 消息报头 响应正文

	

	//'fopen': This function or variable may be unsafe. Consider using fopen_s instead. To disable deprecation, use _CRT_SECURE_NO_WARNINGS. See online help for details.
	FILE* fp = fopen(filepath.c_str(), "rb");
	if (!fp) {
		evhttp_send_reply(request, HTTP_NOTFOUND, "", nullptr);
		return;
	}

	evbuffer* outbuf = evhttp_request_get_output_buffer(request);

	for (;;) {
		int len = fread(buf, 1, sizeof(buf), fp);
		if (len <= 0) break;
		evbuffer_add(outbuf, buf, len);
	}
	evhttp_send_reply(request, HTTP_OK, "", outbuf);
	fclose(fp);
	return;
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
	if (base) {
		std::cout << "event_base_new success" << std::endl;
	}

	//http服务器
	//1 创建evhttp上下文
	evhttp* evh = evhttp_new(base);

	//2 绑定地址和端口
	if (evhttp_bind_socket(evh, "0.0.0.0", 8080) != 0) {
		std::cerr << "evhttp_bind_socket error" << std::endl;
		return 1;
	}

	//3 设置请求回调函数
	evhttp_set_gencb(evh, http_cb, nullptr);

	if (base)
		event_base_dispatch(base);
	if (base)
		event_base_free(base);

#if _WIN32
	WSACleanup();
#endif
	return 0;
}