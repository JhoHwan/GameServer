#pragma once

#define WIN32_LEAN_AND_MEAN // 거의 사용되지 않는 내용을 Windows 헤더에서 제외합니다.
#define _CRT_SECURE_NO_WARNINGS

#include "CorePch.h"

#ifdef _DEBUG
#pragma comment(lib, "libprotobufd.lib")
#else
#pragma comment(lib, "libprotobuf.lib")
#endif
#pragma comment(lib, "ServerCore.lib")

#include "Util/Vector3.h"
