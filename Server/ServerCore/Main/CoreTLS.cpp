#include "CoreTLS.h"

thread_local std::queue<JobQueueRef> LJobQueue;
thread_local JobTimer LJobTimer;
