#include "XThreadPool.h"
#include "XThread.h"
#include <thread>
#include <iostream>
using namespace std;

//分发线程
void XThreadPool::Dispatch(XTask* task) {
	//轮询
	if (!task) return;
	int tid = (lastThread + 1) % threadCount;
	lastThread = tid;
	XThread* t = threads[tid];
	t->AddTask(task);

	//激活线程
	t->Activate();
}

void XThreadPool::Init(int threadCount)
{
	this->threadCount = threadCount;
	for (int i = 0; i < threadCount; i++)
	{
		XThread* thread = new XThread();
		thread->id = i + 1;
		cout << "Creating thread " << i + 1 << endl;
		thread->Start();
		threads.push_back(thread);
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
}
