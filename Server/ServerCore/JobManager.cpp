#include "pch.h"
#include "JobManager.h"

void JobManager::Push(shared_ptr<JobQueue> jobQueue)
{
	lock_guard lock(_lock);
	_queue.push(jobQueue);
}

shared_ptr<JobQueue> JobManager::Pop()
{
	lock_guard lock(_lock);
	if (_queue.empty()) return nullptr;

	auto jobQueue = _queue.front();
	_queue.pop();

	return jobQueue;
}
