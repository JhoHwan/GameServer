#include "pch.h"
#include "DoubleJobQueue.h"
#include <chrono>

DoubleJobQueue::DoubleJobQueue()
{
	_insertJobQueue = new queue<shared_ptr<Job>>();
	_executeJobQueue = new queue<shared_ptr<Job>>();
}

DoubleJobQueue::~DoubleJobQueue()
{
	delete _insertJobQueue;
	delete _executeJobQueue;
}

void DoubleJobQueue::InsertJob(shared_ptr<Job> job)
{
	std::lock_guard<SpinLock> lock(_lock);

	_insertJobQueue->push(job);
}

void DoubleJobQueue::ExecuteJob()
{
	int32 executeJobCount = 0;
	int32 totalJobCount = _executeJobQueue->size();
	while (!_executeJobQueue->empty())
	{
		auto job = _executeJobQueue->front();
		job->Execute();

		_executeJobQueue->pop();
		++executeJobCount;
	}
}

void DoubleJobQueue::SwapQueue()
{
	std::lock_guard<SpinLock> lock(_lock);

	std::swap(_insertJobQueue, _executeJobQueue);
}
