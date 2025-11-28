#pragma once

#define WIN32_LEAN_AND_MEAN // 거의 사용되지 않는 내용을 Windows 헤더에서 제외합니다.

#include "CorePch.h"

#ifdef _DEBUG
#pragma comment(lib, "ServerCore.lib")
#pragma comment(lib, "libprotobufd.lib")
#else
#pragma comment(lib, "ServerCore.lib")
#pragma comment(lib, "libprotobuf.lib")
#endif

#include <nlohmann\json.hpp>
using json = nlohmann::json;

#include "Util/Log/LogMacro.h"