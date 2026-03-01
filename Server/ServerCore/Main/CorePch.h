#pragma once

#include "Types.h"
#include "CoreMacro.h"

#include <array>
#include <vector>
#include <list>
#include <queue>
#include <stack>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <shared_mutex>

#include <iostream>
#include <assert.h>
using namespace std;

#ifdef _WIN32
	#include <windows.h>
	#include <winsock2.h>
	#include <mswsock.h>
	#include <ws2tcpip.h>
	#pragma comment(lib, "ws2_32.lib")
#else
	#include <unistd.h>
	#include <fcntl.h>
	#include <sys/socket.h>
	#include <sys/epoll.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <cstring>
	#include <cerrno>
	#include <netinet/tcp.h>
#endif

#include "CoreTLS.h"
#include "Singleton.h"
#include "SendBuffer.h"
#include "Session.h"
#include "JobQueue.h"

