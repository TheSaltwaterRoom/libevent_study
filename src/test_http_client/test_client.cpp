#include <event2/event.h>
#include <event2/listener.h>
#include <event2/http.h>
#include <event2/bufferevent.h>
#include <event2/keyvalq_struct.h>
#include <event2/buffer.h>
#include <iostream>
#include <string.h>
#include <errno.h>

#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#endif

#ifndef _WIN32
#include <signal.h>
#endif // !_WIN32
using namespace std;

static bool CreateDirIfNotExists(const string& dir)
{
	if (dir.empty()) return true;

#ifdef _WIN32
	if (_mkdir(dir.c_str()) == 0) return true;
#else
	if (mkdir(dir.c_str(), 0755) == 0) return true;
#endif

	return errno == EEXIST;
}

static bool CreateParentDirs(const string& filepath)
{
	size_t pos = 0;
	for (;;) {
		pos = filepath.find_first_of("/\\", pos + 1);
		if (pos == string::npos) break;

		string dir = filepath.substr(0, pos);
		if (dir == "." || dir.empty()) continue;

		if (!CreateDirIfNotExists(dir)) {
			cout << "create dir " << dir << " error" << endl;
			return false;
		}
	}

	return true;
}

void http_client_cb(struct evhttp_request* request, void* arg) {
	cout << "HTTP request received" << endl;
	event_base* base = (event_base*)arg;
	//服务端响应错误
	if (request == NULL) {
		int errcode = EVUTIL_SOCKET_ERROR();
		cout << "socket error:" << evutil_socket_error_to_string(errcode);
		return;
	}

	//获取path
	const char* path = evhttp_request_get_uri(request);
	cout << "path: " << path << endl;
	string filepath = ".";
	filepath += path;
	cout << "filepath is " << filepath << endl;
	//如果路径中有目录，需要分析出目录，并创建
	if (!CreateParentDirs(filepath)) {
		return;
	}

	FILE* fp = fopen(filepath.c_str(), "wb");
	if (!fp) {
		cout << "open file " << filepath<< " error" << endl;
	}

	//获取返回的code 200 404
	cout << "Response：" << evhttp_request_get_response_code(request); //200
	cout << " " << evhttp_request_get_response_code_line(request) << endl;//ok

	char buf[1024] = { 0 };
	evbuffer*input= evhttp_request_get_input_buffer(request);
	for (;;) {
		int len = evbuffer_remove(input, buf, sizeof(buf) - 1);
		if (len <= 0) {
			break;
		}
		buf[len] = '\0';
		if (!fp) continue;
		fwrite(buf, 1, len, fp);
		//cout << buf << flush;
	}

	if (fp) fclose(fp);

	event_base_loopbreak(base);

	return;
}

int TestGetHttp() {
	event_base* base = event_base_new();
	if (base) {
		std::cout << "event_base_new success" << std::endl;
	}

	//生成请求信息GET
	string http_url = "http://www.people.com.cn/index.html?id=1";
	http_url = "http://www.people.com.cn/NMediaFile/2026/0603/MAIN1780455113486I6IT4SS9NI.jpg";

	//分析url地址
	evhttp_uri* uri = evhttp_uri_parse(http_url.c_str());

	const char* scheme = evhttp_uri_get_scheme(uri);
	if (!scheme) {
		std::cerr << "scheme error" << std::endl;
		return 1;
	}
	cout << "scheme: " << scheme << endl;

	int port = evhttp_uri_get_port(uri);
	if (port == -1) {
		if (strcmp(scheme, "http") == 0) {
			port = 80;
		}
		else if (strcmp(scheme, "https") == 0) {
			port = 443;
		}
		else {
			std::cerr << "scheme error" << std::endl;
			return 1;
		}
	}
	cout << "port: " << port << endl;

	const char* host = evhttp_uri_get_host(uri);
	if (!host) {
		std::cerr << "host error" << std::endl;
		return 1;
	}
	cout << "host: " << host << endl;

	const char* path = evhttp_uri_get_path(uri);
	if (!path || strlen(path) == 0) {
		path = "/";
	}
	if (path) {
		cout << "path: " << path << endl;
	}

	const char* query = evhttp_uri_get_query(uri);
	if (query) {
		cout << "query: " << query << endl;
	}

	//bufferevent 连接http服务器
	bufferevent* bev = bufferevent_socket_new(base, -1, BEV_OPT_CLOSE_ON_FREE);
	evhttp_connection* evcon = evhttp_connection_base_bufferevent_new(base, nullptr, bev, host, port);

	//http client请求 回调函数设置
	evhttp_request* rep = evhttp_request_new(http_client_cb, base);

	//设置请求的head消息报头信息
	evkeyvalq* out_headers = evhttp_request_get_output_headers(rep);
	evhttp_add_header(out_headers, "Host", host);

	//发起请求
	evhttp_make_request(evcon, rep, EVHTTP_REQ_GET, path);

	if (base)
		event_base_dispatch(base);
	if (base)
		event_base_free(base);
}

int TestPostHttp() {
	event_base* base = event_base_new();
	if (base) {
		std::cout << "event_base_new success" << std::endl;
	}

	//生成请求信息
	string http_url = "http://127.0.0.1:8080/index.html";

	//分析url地址
	evhttp_uri* uri = evhttp_uri_parse(http_url.c_str());

	const char* scheme = evhttp_uri_get_scheme(uri);
	if (!scheme) {
		std::cerr << "scheme error" << std::endl;
		return 1;
	}
	cout << "scheme: " << scheme << endl;

	int port = evhttp_uri_get_port(uri);
	if (port == -1) {
		if (strcmp(scheme, "http") == 0) {
			port = 80;
		}
		else if (strcmp(scheme, "https") == 0) {
			port = 443;
		}
		else {
			std::cerr << "scheme error" << std::endl;
			return 1;
		}
	}
	cout << "port: " << port << endl;

	const char* host = evhttp_uri_get_host(uri);
	if (!host) {
		std::cerr << "host error" << std::endl;
		return 1;
	}
	cout << "host: " << host << endl;

	const char* path = evhttp_uri_get_path(uri);
	if (!path || strlen(path) == 0) {
		path = "/";
	}
	if (path) {
		cout << "path: " << path << endl;
	}

	const char* query = evhttp_uri_get_query(uri);
	if (query) {
		cout << "query: " << query << endl;
	}

	//bufferevent 连接http服务器
	bufferevent* bev = bufferevent_socket_new(base, -1, BEV_OPT_CLOSE_ON_FREE);
	evhttp_connection* evcon = evhttp_connection_base_bufferevent_new(base, nullptr, bev, host, port);

	//http client请求 回调函数设置
	evhttp_request* rep = evhttp_request_new(http_client_cb, base);

	//设置请求的head消息报头信息
	evkeyvalq* out_headers = evhttp_request_get_output_headers(rep);
	evhttp_add_header(out_headers, "Host", host);

	//发送post数据
	evbuffer* output_buffer = evhttp_request_get_output_buffer(rep);
	evbuffer_add_printf(output_buffer, "name=%s&age=%d", "zhangsan", 20);

	//发起请求
	evhttp_make_request(evcon, rep, EVHTTP_REQ_POST, path);



	if (base)
		event_base_dispatch(base);
	if (base)
		event_base_free(base);
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
	TestGetHttp();
	TestPostHttp();
	

#if _WIN32
	WSACleanup();
#endif
	return 0;
}
