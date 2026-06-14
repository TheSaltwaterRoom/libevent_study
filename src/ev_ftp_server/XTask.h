#pragma once
class XTask
{
public:
	struct event_base *base = nullptr;
	int sock = 0;
	int thread_id = 0;
	virtual ~XTask() = default;
	virtual bool Init() = 0;
};
