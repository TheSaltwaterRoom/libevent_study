#include "XFtpLIST.h"
#include <iostream>
#include <event2/event.h>
#include <event2/bufferevent.h>
#include <string>
#ifdef _WIN32
#include <io.h>
#endif // _WIN32

using namespace std;

void XFtpLIST::Write(struct bufferevent *bev)
{
	ResCMD("226 Transfer complete\r\n");
	Close();

}
void XFtpLIST::Event(struct bufferevent *bev, short what)
{
	if (what & BEV_EVENT_EOF)
	{
		cout << "XFtpLIST BEV_EVENT_EOF" << endl;
		Close();
	}
	else if (what & (BEV_EVENT_ERROR | BEV_EVENT_TIMEOUT))
	{
		cout << "XFtpLIST data connection failed" << endl;
		Close();
		ResCMD("425 Cannot open data connection\r\n");
	}
	else if(what & BEV_EVENT_CONNECTED)
	{
		cout << "XFtpLIST BEV_EVENT_CONNECTED" << endl;
	}
}

void XFtpLIST::Parse(std::string type, std::string msg)
{
	string resmsg;

	if (type == "PWD")
	{
		//257 "/" is current directory.
		resmsg = "257 \"";
		resmsg += cmdTask->curDir;
		resmsg += "\" is current dir.\r\n";

		ResCMD(resmsg);
	}
	else if (type == "LIST")
	{
		//-rwxrwxrwx 1 root group 64463 Mar 14 09:53 101.jpg\r\n
		if (!ConnectPORT())
		{
			ResCMD("425 Cannot open data connection\r\n");
			return;
		}
		//2 1502 150
		ResCMD("150 Here comes the directory listing.\r\n");
		//string listdata = "-rwxrwxrwx 1 root group 64463 Mar 14 09:53 101.jpg\r\n";
		string listdata = GetListData(cmdTask->rootDir + cmdTask->curDir);
		Send(listdata);
	}
	else if (type == "CWD")
	{
		//CWD test\r\n
		int pos =static_cast<int> (msg.rfind(' ') + 1);

		string path = msg.substr(pos, msg.size() - pos - 2);
		if (path.empty())
		{
			ResCMD("550 Invalid directory.\r\n");
			return;
		}

		if (path[0] == '/')
		{
			cmdTask->curDir = path;
		}
		else
		{
			if (cmdTask->curDir[cmdTask->curDir.size() - 1] != '/')
            {
			    cmdTask->curDir += "/";
            }
			cmdTask->curDir += path + "/";
		}
		//  /test/
		ResCMD("250 Directory succes chanaged.\r\n");
	}
	else if (type == "CDUP")
	{
		string& path = cmdTask->curDir;
		if (path.empty() || path == "/")
		{
			path = "/";
			ResCMD("250 Directory successfully changed.\r\n");
			return;
		}

		while (path.size() > 1 && path.back() == '/')
		{
			path.pop_back();
		}

		size_t pos = path.rfind('/');
		if (pos == string::npos || pos == 0)
		{
			path = "/";
		}
		else
		{
			path = path.substr(0, pos + 1);
		}

		ResCMD("250 Directory successfully changed.\r\n");
	}
}
std::string XFtpLIST::GetListData(std::string path)
{
	//-rwxrwxrwx 1 root group 64463 Mar 14 09:53 101.jpg\r\n
	string data;
#if _WIN32
	_finddata_t file;

	path += "/*.*";

	intptr_t dir = _findfirst(path.c_str(), &file);
	if (dir < 0)
	{
		return data;
	}

	do {
		string tmp = "";
		if (file.attrib & _A_SUBDIR)
		{
			if (strcmp(file.name, ".") == 0 ||
				strcmp(file.name, "..") == 0)
				continue;

			tmp = "drwxrwxrwx 1 root group ";
		}
		else
		{
			tmp = "-rwxrwxrwx 1 root group ";
		}

		//文件大小
		char buf[1024] = { 0 };

		sprintf_s(buf, sizeof(buf), "%lld ",
			static_cast<long long>(file.size));
		tmp += buf;

		//文件时间
		tm localTime{};
		localtime_s(&localTime, &file.time_write);

		strftime(
			buf,
			sizeof(buf),
			"%b %d %H:%M ",
			&localTime
		);
		tmp += buf;

		tmp += file.name;
		tmp += "\r\n";

		//cout << "tmp: " << tmp << endl;
		data += tmp;

	} while (_findnext(dir, &file) == 0);

#else
	string cmd = "ls -l ";
	cmd += path;
	cout << "popen:" << cmd << endl;
	FILE* f = popen(cmd.c_str(), "r");
	if (!f)
	{
		return data;
	}
	char buffer[1024] = { 0 };
	for (;;)
	{
		int len = static_cast<int>(fread(buffer, 1, sizeof(buffer) - 1, f));
		if (len <= 0)
		{
			break;
		}
		buffer[len] = '\0';
		data += buffer;
	}
	pclose(f);
#endif // !_WIN32


	return data;
}
