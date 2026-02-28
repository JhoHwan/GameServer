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
	using SOCKET = int;
	const int INVALID_SOCKET = -1;
	const int SOCKET_ERROR = -1;
	using HANDLE = int;
	const int INFINITE = -1;
	using LINGER = struct linger;

	using SOCKADDR_IN = struct sockaddr_in;
	using IN_ADDR = struct in_addr;
	using WCHAR = wchar_t;
	using BYTE = uint8_t;

	#include <unistd.h>
	#include <fcntl.h>
	#include <sys/socket.h>
	#include <sys/epoll.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <cstring>
	#include <cerrno>
#endif

#include "CoreTLS.h"
#include "Singleton.h"
#include "SendBuffer.h"
#include "Session.h"
#include "JobQueue.h"

