#pragma once
class JobManager
{
public:
	void Push(shared_ptr<JobQueue>);
	shared_ptr<JobQueue> Pop();

private:
	mutex _lock;
	queue<shared_ptr<JobQueue>> _queue;
};

