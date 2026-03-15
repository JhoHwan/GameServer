#include "JobQueue.h"


moodycamel::ConcurrentQueue<JobQueueRef> GGlobalJobQueue;

/*--------------
	JobQueue
---------------*/

void JobQueue::Push(JobRef job)
{
	_jobs.enqueue(job);
	_jobCount.fetch_add(1);

	bool expected = false;
	if (_isExecute.compare_exchange_strong(expected, true))
	{
		LJobQueue.push(shared_from_this());
	}
}

int32 JobQueue::Execute()
{
	auto start = GetTickCount64();
	int processedJobCount = 0;
	while (true)
	{
		auto now = GetTickCount64();
		if(now - start > 10)
		{
			GGlobalJobQueue.enqueue(shared_from_this());
			break;
		}

		JobRef jobs[64];
		const auto dequeueJobCount = static_cast<int>(_jobs.try_dequeue_bulk(jobs, 64));

		for(int i = 0; i < dequeueJobCount; i++)
		{
			jobs[i]->Execute();
			jobs[i].reset();
		}
		processedJobCount += dequeueJobCount;

		if (_jobCount.fetch_sub(dequeueJobCount) == dequeueJobCount)
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
			break;
		}
	}

	return processedJobCount;
}
