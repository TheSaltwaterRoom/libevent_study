#include "XFtpSTOR.h"
#include <iostream>
#include <event2/bufferevent.h>
#include <event2/event.h>
using namespace std;

void XFtpSTOR::Parse(std::string type, std::string msg)
{
	int pos = static_cast<int>(msg.rfind(' ') + 1);
	string filename = msg.substr(pos, msg.size() - pos - 2);

	string path = cmdTask->rootDir;
	path += cmdTask->curDir;
	path += filename;

	fp = fopen(path.c_str(), "wb");
	if (!fp)
	{
		ResCMD("450 File open failed\r\n");
		return;
	}

	if (!ConnectPORT())
	{
		Close();
		ResCMD("425 Cannot open data connection\r\n");
		return;
	}

	ResCMD("150 File status okay; about to open data connection\r\n");
}
void XFtpSTOR::Read(struct bufferevent *bev)
{
	if (!fp)
    {
	    return;
    }
	for (;;)
	{
		int len =static_cast<int>(bufferevent_read(bev, buf, sizeof(buf)));
		if (len <= 0)
        {
		    return;
        }
		int size =static_cast<int>(fwrite(buf, 1, len, fp));
		cout << "<"<<len<<":"<<size << ">" << flush;
	}
}
void XFtpSTOR::Event(bufferevent* bev, short what)
{
    if (what & BEV_EVENT_EOF)
    {
        cout << "XFtpSTOR BEV_EVENT_EOF" << endl;

        // 处理输入缓冲区最后剩余的数据
        Read(bev);

        // 关闭数据连接，并通过fclose刷新文件
        Close();

        ResCMD("226 Transfer complete\r\n");
        return;
    }

    if (what & (BEV_EVENT_ERROR | BEV_EVENT_TIMEOUT))
    {
        cout << "XFtpSTOR transfer failed" << endl;
        Close();
        ResCMD("426 Transfer aborted\r\n");
        return;
    }

    if (what & BEV_EVENT_CONNECTED)
    {
        cout << "XFtpSTOR BEV_EVENT_CONNECTED" << endl;
    }
}
