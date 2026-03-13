#pragma once

#include <filesystem>

#include "DetourNavMesh.h"

class NavMeshLoader
{
public:
    static dtNavMesh* LoadNavMeshFromBin(const std::filesystem::path& path);

private:
    static constexpr int NAVMESHSET_MAGIC = 'M' << 24 | 'S' << 16 | 'E' << 8 | 'T'; //'MSET';
    static constexpr int NAVMESHSET_VERSION = 1;

    struct NavMeshSetHeader
    {
        int32_t magic;
        int32_t version;
        int32_t numTiles;
        dtNavMeshParams params;
    };

    struct NavMeshTileHeader
    {
        dtTileRef tileRef;
        int32_t dataSize;
    };
};
