#include <event2/event.h>
#include <event2/listener.h>
#include <zlib.h>
#include <iostream>
#include <string.h>

#ifndef _WIN32
#include <signal.h>
#endif // !_WIN32


#define SPORT 5001

using namespace std;

void Server(event_base* base);
void Client(event_base* base);
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
	if(base){
		std::cout << "event_base_new success" << std::endl;
	}

	Server(base);
	Client(base);

	if(base)
		event_base_dispatch(base);
	if(base)
		event_base_free(base);
#if _WIN32
	WSACleanup();
#endif
	return 0;
}