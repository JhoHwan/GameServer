#pragma once
#include "Job.h"
#include "LockQueue.h"
#include "JobTimer.h"

#include <concurrent_queue.h>


extern Concurrency::concurrent_queue<JobQueueRef> GGlobalJobQueue;

/*--------------
	JobQueue
---------------*/

class JobQueue : public enable_shared_from_this<JobQueue>
{
public:
	void DoAsync(CallbackType&& callback)
	{
		Push(make_shared<Job>(std::move(callback)));
	}

	template<typename T, typename Ret, typename... Args>
	void DoAsync(Ret(T::*memFunc)(Args...), Args... args)
	{
		shared_ptr<T> owner = static_pointer_cast<T>(shared_from_this());
		Push(make_shared<Job>(owner, memFunc, std::forward<Args>(args)...));
	}

	void ClearJobs() { _jobs.clear(); }

public:
	void	Push(JobRef job);
	void	Execute(int32 excuteTime);

protected:
	//LockQueue<JobRef> _jobs;

	Concurrency::concurrent_queue<JobRef> _jobs;
	atomic<bool> _isExecute;
	atomic<int32> _jobCount;
};

