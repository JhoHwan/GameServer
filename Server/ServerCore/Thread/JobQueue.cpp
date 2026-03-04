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

void JobQueue::Execute(int32 executeCount)
{
	int currentJobCount = 0;
	while (true)
	{
		JobRef jobs[32];
		int32 cnt = 0;

		const auto jobCount = static_cast<int>(_jobs.try_dequeue_bulk(jobs, 32));

		for(int i = 0; i < jobCount; i++)
		{
			jobs[i]->Execute();
			jobs[i].reset();
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

		currentJobCount += jobCount;
		if (currentJobCount >= executeCount)
		{
			GGlobalJobQueue.enqueue(shared_from_this()); // 글로벌 잡큐 추가 후 해제
			return;
		}
	}
}
