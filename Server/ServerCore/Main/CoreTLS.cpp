#include "CoreTLS.h"

thread_local std::queue<JobQueueRef> LJobQueue;
thread_local JobTimer LJobTimer;
thread_local uint32 LThreadId;