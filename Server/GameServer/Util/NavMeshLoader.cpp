#include "pch.h"
#include "NavMeshLoader.h"

#include <fstream>
#include <iostream>

#include "LogManager.h"

dtNavMesh* NavMeshLoader::LoadNavMeshFromBin(const char* path)
{
    LOG_DEBUG("[NavMesh] Trying to load NavMesh from: {}", path);

    std::FILE* fp = std::fopen(path, "rb");
    if (!fp) 
    {
        LOG_ERROR("[NavMesh] Failed to open file: {}", path);
        return nullptr;
    }

    // Read header.
    NavMeshSetHeader header{};
    size_t sizenum = sizeof(NavMeshSetHeader);
    size_t readLen = std::fread(&header, sizenum, 1, fp);
    if (readLen != 1)
    {
        LOG_ERROR("[NavMesh] Failed to read NavMeshSetHeader.");
        std::fclose(fp);
        return nullptr;
    }
    if (header.magic != NAVMESHSET_MAGIC)
    {
        LOG_ERROR("[NavMesh] Magic number mismatch. Expected: {}, Got: {}", NAVMESHSET_MAGIC, header.magic);
        std::fclose(fp);
        return nullptr;
    }
    if (header.version != NAVMESHSET_VERSION)
    {
        LOG_ERROR("[NavMesh] Version mismatch. Expected: {}, Got: {}", NAVMESHSET_VERSION, header.version);
        std::fclose(fp);
        return nullptr;
    }

    LOG_DEBUG("[NavMesh] Header loaded. Tile count: {}", header.numTiles);

    dtNavMesh* mesh = dtAllocNavMesh();
    if (!mesh)
    {
        LOG_ERROR("[NavMesh] Failed to allocate dtNavMesh.");
        std::fclose(fp);
        return nullptr;
    }

    dtStatus status = mesh->init(&header.params);
    if (dtStatusFailed(status))
    {
        LOG_ERROR("[NavMesh] Failed to init dtNavMesh. Status: 0x{:X}", status);
        LOG_ERROR(" - orig: [{}, {}, {}]", header.params.orig[0], header.params.orig[1], header.params.orig[2]);
        LOG_ERROR(" - tileWidth: {}, tileHeight: {}", header.params.tileWidth, header.params.tileHeight);
        LOG_ERROR(" - maxTiles: {}, maxPolys: {}", header.params.maxTiles, header.params.maxPolys);
        dtFreeNavMesh(mesh);
        std::fclose(fp);
        return nullptr;
    }

    // Read tiles.
    int successTiles = 0;
    for (int i = 0; i < header.numTiles; ++i)
    {
        NavMeshTileHeader tileHeader{};
        readLen = std::fread(&tileHeader, sizeof(tileHeader), 1, fp);
        if (readLen != 1)
        {
            LOG_ERROR("[NavMesh] Failed to read NavMeshTileHeader for tile index {}", i);
            dtFreeNavMesh(mesh);
            std::fclose(fp);
            return nullptr;
        }

        if (!tileHeader.tileRef || !tileHeader.dataSize)
        {
            LOG_WARN("[NavMesh] Invalid tileRef or dataSize at index {}. Stopping tile read.", i);
            break;
        }

        unsigned char* data = (unsigned char*)dtAlloc(tileHeader.dataSize, DT_ALLOC_PERM);
        if (!data) 
        {
            LOG_ERROR("[NavMesh] Failed to allocate tile data. Size: {}", tileHeader.dataSize);
            break;
        }
        std::memset(data, 0, tileHeader.dataSize);
        readLen = fread(data, tileHeader.dataSize, 1, fp);
        if (readLen != 1)
        {
            LOG_ERROR("[NavMesh] Failed to read tile data for tile index {}", i);
            dtFree(data);
            dtFreeNavMesh(mesh);
            fclose(fp);
            return nullptr;
        }

        status = mesh->addTile(data, tileHeader.dataSize, DT_TILE_FREE_DATA, tileHeader.tileRef, nullptr);
        if (dtStatusFailed(status))
        {
            LOG_ERROR("[NavMesh] Failed to add tile for index {}", i);
            dtFree(data);
            continue;
        }
        successTiles++;
    }

    std::fclose(fp);

    LOG_DEBUG("[NavMesh] Successfully loaded NavMesh. Loaded tiles: {}/{}", successTiles, header.numTiles);

    return mesh;
}