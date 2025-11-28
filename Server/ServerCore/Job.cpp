#include "pch.h"
#include "Job.h"

Job::Job(Function&& func)
{
	_func = std::move(func);
}

void Job::Execute() const
{
	if (_func)
	{
		_func();
	}
}

void Job::operator() () const
{
	Execute();
}