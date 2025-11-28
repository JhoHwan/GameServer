#include "pch.h"
#include "JobQueue.h"

void JobQueue::PushJob(shared_ptr<Job> job)
{
	lock_guard lock(_lock);

	_queue.push(job);
	GJobManager->Push(shared_from_this());
}

bool JobQueue::TryExcute()
{
	if (_isProcessing.exchange(true) == true) return false;

	vector<shared_ptr<Job>> jobs;
	{
		lock_guard lock(_lock);
		while (!_queue.empty())
		{
			jobs.push_back(_queue.front());
			_queue.pop();
		}
	}

	for (const auto& job : jobs)
	{
		job->Execute();
	}
	jobs.clear();

	_isProcessing.store(false);
	return true;
}


