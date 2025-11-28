#pragma once

#include "Type.h"

#include <memory>
#include <atomic>
#include <iostream>
#include <thread>
#include <algorithm>
#include <vector>
#include <queue>
#include <stack>
#include <array>
#include <set>
#include <map>
#include <mutex>
#include <functional>
#include <limits>
#include <shared_mutex>
#include <chrono>

#include <WinSock2.h>
#include <WS2tcpip.h>
#include <mswsock.h>
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib,"mswsock.lib")

using namespace std;

#include "ThreadPool.h"
#include "SpinLock.h"
#include "SocketUtil.h"

#include "RecvBuffer.h"
#include "IOCPCore.h"
#include "IOCPObject.h"

#include "SendBuffer.h"
#include "IPacket.h"
//#include "PacketHandler.h"
#include "IOCPServer.h"
#include "Job.h"
#include "JobQueue.h"
#include "DoubleJobQueue.h"
#include "JobManager.h"
#include "Connector.h"

#include "CoreGlobal.h"







