#pragma once

#define WIN32_LEAN_AND_MEAN // 거의 사용되지 않는 내용을 Windows 헤더에서 제외합니다.
#define _CRT_SECURE_NO_WARNINGS
#include <sw/redis++/redis++.h>

#include "CorePch.h"

#ifdef _DEBUG
#pragma comment(lib, "libprotobufd.lib")
#pragma comment(lib, "hiredisd.lib")
#else
#pragma comment(lib, "libprotobuf.lib")
#pragma comment(lib, "hiredis.lib")
#endif
#pragma comment(lib, "redis++_static.lib")
#pragma comment(lib, "ServerCore.lib")

#include "Util\Vector3.h"
