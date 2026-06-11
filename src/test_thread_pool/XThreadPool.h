#pragma once
#include <vector>
class XThread;
class XTask;

class XThreadPool
{
public:
	static XThreadPool* GetInstance()
	{
		static XThreadPool instance;
		return &instance;
	}

	//初始化线程池并启动线程
	void Init(int threadCount);

	//分发线程
	void Dispatch(XTask* task);
private:
	XThreadPool() = default;
	~XThreadPool() = default;
	XThreadPool(const XThreadPool&) = delete;
	XThreadPool& operator=(const XThreadPool&) = delete;

	//线程池中的线程数量
	int threadCount = 0;
	int lastThread = -1;
	std::vector<XThread *> threads;
};

