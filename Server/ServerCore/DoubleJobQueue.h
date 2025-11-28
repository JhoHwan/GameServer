#pragma once

class DoubleJobQueue
{
public:
	DoubleJobQueue();
	~DoubleJobQueue();

	void InsertJob(shared_ptr<Job> job);

	void ExecuteJob();

	void SwapQueue();

private:
	SpinLock _lock;

	std::queue<shared_ptr<Job>>* _insertJobQueue;
	std::queue<shared_ptr<Job>>* _executeJobQueue;

};
