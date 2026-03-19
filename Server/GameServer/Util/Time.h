#pragma once
#include "CorePch.h"
#include "Types.h"

namespace Time
{
    inline uint64 GServerStartTime;
    inline void InitServerStartTime() {GServerStartTime = GetTickCount64();}
    inline uint64 GetServerTime() { return GetTickCount64() - GServerStartTime; }
}
