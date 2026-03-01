#pragma once
#include "Job.h"
#include "JobTimer.h"

#include "concurrentqueue.h"


extern moodycamel::ConcurrentQueue<JobQueueRef> GGlobalJobQueue;

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

public:
	void	Push(JobRef job);
	void	Execute(int32 excuteTime);

protected:
	//LockQueue<JobRef> _jobs;

	moodycamel::ConcurrentQueue<JobRef> _jobs;
	atomic<bool> _isExecute;
	atomic<int32> _jobCount;
};

