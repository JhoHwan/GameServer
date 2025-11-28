#include "pch.h"
#include "JobQueue.h"

/*--------------
	JobQueue
---------------*/

void JobQueue::Push(JobRef job, bool pushOnly)
{
	const int32 prevCount = _jobCount.fetch_add(1);
	_jobs.Push(job); // WRITE_LOCK
}
