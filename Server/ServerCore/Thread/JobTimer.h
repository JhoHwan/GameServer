#pragma once

struct JobData
{
	JobData(weak_ptr<JobQueue> owner, JobRef job) : owner(owner), job(job)
	{

	}

	weak_ptr<JobQueue>	owner;
	JobRef				job;
};

struct JobCancelToken
{
public:
	void CancelJob() { _isCanceld = true; }
	bool IsCanceled() const { return _isCanceld; }

private:
	bool _isCanceld = false;
};

struct TimerItem
{
	TimerItem(uint64 executeTick, JobData* jobData) 
		: executeTick(executeTick),
		jobData(jobData),
		cancelToken(make_shared<JobCancelToken>()) {}

	bool operator<(const TimerItem& other) const
	{
		return executeTick > other.executeTick;
	}

	uint64 executeTick = 0;
	JobData* jobData = nullptr;
	shared_ptr<JobCancelToken> cancelToken;
};

/*--------------
	JobTimer
---------------*/

class JobTimer : public Singleton<JobTimer>
{
public:
	shared_ptr<JobCancelToken>			Reserve(uint64 tickAfter, weak_ptr<JobQueue> owner, JobRef job);
	void								Distribute(uint64 now);
	void								Clear();

private:
	USE_LOCK;
	priority_queue<TimerItem>	_items;
	atomic<bool>				_distributing = false;
};

extern JobTimer& GJobTimer;