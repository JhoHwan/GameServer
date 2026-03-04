#pragma once

#define CRT_SECURE_NO_WARNINGS

#include "CorePch.h"

#ifdef _DEBUG
#pragma comment(lib, "libprotobufd.lib")
#else
#pragma comment(lib, "libprotobuf.lib")
#endif
#pragma comment(lib, "ServerCore.lib")

#include "Util/Vector3.h"
