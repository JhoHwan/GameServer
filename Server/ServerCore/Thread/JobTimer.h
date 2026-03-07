#pragma once

struct JobData
{
	JobData(weak_ptr<JobQueue> owner, JobRef job) : owner(owner), job(job)
	{

	}

	weak_ptr<JobQueue>	owner;
	JobRef				job;
};

struct TimerItem
{
	TimerItem(uint64 executeTick, JobData jobData)
		: executeTick(executeTick),
		jobData(std::move(jobData)){}

	bool operator<(const TimerItem& other) const
	{
		return executeTick > other.executeTick;
	}

	uint64 executeTick = 0;
	JobData jobData;
};

/*--------------
	JobTimer
---------------*/

class JobTimer
{
public:
	void								Reserve(uint64 tickAfter, JobQueueRef owner, const JobRef& job);
	void								Distribute(uint64 now);
	void								Clear();


private:
	priority_queue<TimerItem>	_items;
	//atomic<bool>				_distributing = false;
};