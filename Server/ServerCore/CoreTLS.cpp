#include "pch.h"
#include "CoreTLS.h"

thread_local std::queue<JobQueueRef> LJobQueue;