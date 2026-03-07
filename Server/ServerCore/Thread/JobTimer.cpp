#include "JobTimer.h"
#include "JobQueue.h"
#include "LogManager.h"

/*--------------
	JobTimer
---------------*/
void JobTimer::Reserve(uint64 tickAfter, JobQueueRef owner, const JobRef& job)
{
	const uint64 currentTick = GetTickCount64();
	const uint64 executeTick = currentTick + tickAfter;
	_items.emplace(executeTick, JobData{std::move(owner), job});
	//LOG_DEBUG("[JobTimer] Reserve Job after {}", tickAfter);
}

void JobTimer::Distribute(uint64 now)
{
	while (_items.empty() == false)
	{
		const TimerItem& timerItem = _items.top();
		if (now < timerItem.executeTick) break;

		if (const JobQueueRef owner = timerItem.jobData.owner.lock())
			owner->Push(timerItem.jobData.job);

		_items.pop();
		//LOG_DEBUG("[JobTimer] Distribute Job {}", now);
	}
}

void JobTimer::Clear()
{
	while (!_items.empty())
	{
		_items.pop();
	}
}
