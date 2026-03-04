#pragma once

extern thread_local std::queue<JobQueueRef> LJobQueue;
extern thread_local class JobTimer LJobTimer;
