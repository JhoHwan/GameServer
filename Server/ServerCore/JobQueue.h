#pragma once

class JobQueue : public enable_shared_from_this<JobQueue>
{
public:
	void PushJob(shared_ptr<Job> job);
	bool TryExcute();

private:
	inline bool IsEmpty() const
	{
		return _queue.empty();
	}

private:
	queue<shared_ptr<Job>>  _queue;
	atomic<bool> _isProcessing;
	mutex _lock;
};

