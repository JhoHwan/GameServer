#include "pch.h"
#include "JobQueue.h"


moodycamel::ConcurrentQueue<JobQueueRef> GGlobalJobQueue;

/*--------------
	JobQueue
---------------*/

void JobQueue::Push(JobRef job)
{
	_jobCount.fetch_add(1);
	_jobs.enqueue(job);

	bool expected = false;
	if (_isExecute.compare_exchange_strong(expected, true))
	{
		LJobQueue.push(shared_from_this());
	}
}

void JobQueue::Execute(int32 excuteTime)
{
	auto startTime = chrono::steady_clock::now();

	while (true)
	{
		vector<JobRef> jobs;
		JobRef job;
		int32 cnt = 0;
		while (_jobs.try_dequeue(job))
		{
			if (job) jobs.push_back(job);
			cnt += 1;
			if (cnt == 32) break;
		}

		const int32 jobCount = static_cast<int32>(jobs.size());

		for (auto job : jobs)
		{
			job->Execute();
			job.reset();
		}

		if (_jobCount.fetch_sub(jobCount) == jobCount)
		{
			_isExecute.store(false);

			if (_jobCount.load() > 0)
			{
				bool expected = false;
				if (_isExecute.compare_exchange_strong(expected, true))
				{
					continue;
				}
			}

			return;
		}

		auto now = std::chrono::steady_clock::now();
		auto duration = chrono::duration_cast<std::chrono::milliseconds>(now - startTime);
		if (duration.count() >= excuteTime)
		{
			GGlobalJobQueue.enqueue(shared_from_this()); // 글로벌 잡큐 추가 후 해제
			return;
		}
	}
}
