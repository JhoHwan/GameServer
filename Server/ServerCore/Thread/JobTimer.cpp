#include "JobTimer.h"
#include "JobQueue.h"

/*--------------
	JobTimer
---------------*/
JobTimer& GJobTimer = JobTimer::Instance();

shared_ptr<JobCancelToken> JobTimer::Reserve(uint64 tickAfter, weak_ptr<JobQueue> owner, JobRef job)
{
	const uint64 executeTick = ::GetTickCount64() + tickAfter;
	JobData* jobData = new JobData(owner, job);
	TimerItem item(executeTick, jobData);
	{
		WRITE_LOCK;
		_items.push(item);
	}
	
	return item.cancelToken;
}

void JobTimer::Distribute(uint64 now)
{
	// 한 번에 1 쓰레드만 통과
	if (_distributing.exchange(true) == true)
		return;

	vector<TimerItem> items;

	{
		WRITE_LOCK;

		while (_items.empty() == false)
		{
			const TimerItem& timerItem = _items.top();
			if (now < timerItem.executeTick)
				break;

			items.push_back(timerItem);
			_items.pop();
		}
	}

	for (TimerItem& item : items)
	{
		if (item.cancelToken->IsCanceled() == false)
		{
			if (JobQueueRef owner = item.jobData->owner.lock())
				owner->Push(item.jobData->job);
		}
		delete item.jobData;		
	}

	// 끝났으면 풀어준다
	_distributing.store(false);
}

void JobTimer::Clear()
{
	WRITE_LOCK;

	while (_items.empty() == false)
	{
		const TimerItem& timerItem = _items.top();
		delete timerItem.jobData;
		_items.pop();
	}
}
