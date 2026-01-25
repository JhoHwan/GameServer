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

#include <windows.h>
#include <iostream>
#include <assert.h>
using namespace std;

#include <winsock2.h>
#include <mswsock.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#include "CoreTLS.h"
#include "Singleton.h"
#include "SendBuffer.h"
#include "Session.h"
#include "JobQueue.h"

