#include "pch.h"
#include "NavMeshLoader.h"

#include <fstream>
#include <iostream>

dtNavMesh* NavMeshLoader::LoadNavMeshFromBin(const char* FilePath)

{
    std::ifstream is(FilePath, std::ios::binary);
    if (!is.is_open())
    {
        std::cerr << "[Nav] File Open Error: " << FilePath << std::endl;
        return nullptr;
    }

    // 1. 헤더 읽기
    FNavMeshFileHeader Header;
    is.read((char*)&Header, sizeof(FNavMeshFileHeader));

    if (Header.Magic != 0xDEADBEEF)
    {
        std::cerr << "[Nav] Invalid Magic Number" << std::endl;
        return nullptr;
    }

    // 2. dtNavMesh 생성 및 초기화
    dtNavMesh* Mesh = dtAllocNavMesh();
    if (!Mesh) return nullptr;

    dtStatus Status = Mesh->init(&Header.Params);
    if (dtStatusFailed(Status))
    {
        std::cerr << "[Nav] Init Failed" << std::endl;
        dtFreeNavMesh(Mesh);
        return nullptr;
    }

    // 3. 타일 데이터 읽어서 복원
    for (int i = 0; i < Header.TileCount; ++i)
    {
        int DataSize = 0;

        // (A) 데이터 사이즈 읽기
        is.read((char*)&DataSize, sizeof(int));
        if (DataSize <= 0) continue;

        // (B) 메모리 할당 
        unsigned char* TileData = (unsigned char*)dtAlloc(DataSize, DT_ALLOC_PERM_NAVMESH);
        if (!TileData) break;

        // (C) 데이터 본문 읽기
        is.read((char*)TileData, DataSize);

        // (D) 타일 추가
        Status = Mesh->addTile(TileData, DataSize, DT_TILE_FREE_DATA, 0, nullptr);

        if (dtStatusFailed(Status))
        {
            std::cerr << "[Nav] AddTile Failed at index " << i << std::endl;
            dtFree(TileData, DT_ALLOC_PERM_NAVMESH); 
        }
    }

    std::cout << "[Nav] Loaded Successfully. Tiles: " << Header.TileCount << std::endl;
    return Mesh;
}

void NavMeshLoader::TestFindPath(dtNavMeshQuery* NavQuery, dtReal StartPos[3], dtReal EndPos[3])

{
    if (!NavQuery) return;

    dtQueryFilter Filter; // 기본 필터 (이동 비용, 갈 수 있는 지역 등 설정)
    // 필요하면 Filter.setIncludeFlags(0xffff); 등으로 설정 가능

    // 검색 범위 (내 좌표에서 상하좌우 위아래로 얼마만큼 떨어진 곳까지 네비메시를 찾을지)
    // 언리얼 좌표계 기준이라면 꽤 넉넉하게 줘야 함. (예: 100, 100, 100)
    dtReal Extents[3] = { 10.0, 100.0, 10.0 };

    dtPolyRef StartPolyRef = 0;
    dtPolyRef EndPolyRef = 0;
    dtReal StartNearestPt[3];
    dtReal EndNearestPt[3];

    // (A) 시작점 근처의 폴리곤 찾기
    NavQuery->findNearestPoly(StartPos, Extents, &Filter, &StartPolyRef, StartNearestPt);

    // (B) 도착점 근처의 폴리곤 찾기
    NavQuery->findNearestPoly(EndPos, Extents, &Filter, &EndPolyRef, EndNearestPt);

    if (!StartPolyRef || !EndPolyRef)
    {
        std::cout << "[Path] Failed to find start or end polygon on NavMesh!" << std::endl;
        return;
    }

    // (C) 경로 계산 (폴리곤 리스트 구하기)
    static const int MAX_POLYS = 256;
    dtQueryResult result;
    int PathCount = 0;
    dtReal costLimit = DBL_MAX;
    dtReal totalCost;

    NavQuery->findPath(StartPolyRef, EndPolyRef, StartNearestPt, EndNearestPt, costLimit, &Filter, result, &totalCost);


    // (D) 실제 이동 좌표 구하기 (String Pulling)
    // 폴리곤 ID만으로는 이동을 못하니까, 실제 꺾이는 지점(Waypoints) 좌표를 뽑아야 함
    static const int MAX_SMOOTH = 256;
    unsigned char StraightPathFlags[MAX_SMOOTH];
    dtPolyRef StraightPathRefs[MAX_SMOOTH];
    result.copyRefs(StraightPathRefs, result.size());


    dtQueryResult pathResult;
    NavQuery->findStraightPath(StartNearestPt, EndNearestPt, StraightPathRefs, result.size(), pathResult);
    for (int i = 0; i < pathResult.size(); i++)
    {
        auto* pos = pathResult.getPos(i);
        printf("[%d] : [%f, %f, %f]\n", i, pos[0], pos[1], pos[2]);
    }
}
