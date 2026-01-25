#pragma once

#include "DetourNavMesh.h"
#include "DetourAlloc.h" 
#include "DetourNavMeshQuery.h"

class NavMeshLoader
{
public:
    static dtNavMesh* LoadNavMeshFromBin(const char* FilePath);

    static void TestFindPath(dtNavMeshQuery* NavQuery, dtReal StartPos[3], dtReal EndPos[3]);

private:
    struct FNavMeshFileHeader
    {
        int32 Magic;        // 파일 식별용 (0xDEADBEEF 등)
        int32 Version;      // 버전 관리용
        int32 TileCount;    // 저장된 타일 개수
        dtNavMeshParams Params; // 네비메시 설정 원본
    };

};