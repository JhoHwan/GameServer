#include "pch.h"
#include "CoreGlobal.h"

JobManager* GJobManager;

class CoreGlobal
{
public:
	CoreGlobal()
	{
		GJobManager = new JobManager();
	}

	~CoreGlobal()
	{
		delete GJobManager;
	}
};