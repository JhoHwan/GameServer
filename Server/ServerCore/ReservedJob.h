#pragma once

class ReservedJob
{
public:
	ReservedJob(int32 ms, shared_ptr<Job> job) : _job(job)
	{
		_executeTime = std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(ms);
	}
	
	void Execute()
	{
		if (_job)
		{
			_job->Execute();
		}
	}

	void operator() ()
	{
		Execute();
	}

	inline bool IsTimeOut(std::chrono::time_point<std::chrono::high_resolution_clock> now) const
	{
		return now >= _executeTime;
	}

	inline auto GetExcuteTime() const
	{
		return _executeTime;
	}

private:
	shared_ptr<Job> _job;
	std::chrono::time_point<std::chrono::high_resolution_clock> _executeTime;
};

struct ReservedJobCompare
{
	bool operator() (shared_ptr<ReservedJob> lhs, shared_ptr<ReservedJob> rhs)
	{
		return lhs->GetExcuteTime() > rhs->GetExcuteTime();
	}
};

using ReservedJobQueue = priority_queue<shared_ptr<ReservedJob>, vector<shared_ptr<ReservedJob>>, ReservedJobCompare>;
