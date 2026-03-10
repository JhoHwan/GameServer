#include "pch.h"
#include "FieldData.h"
#include "Util/NavMeshLoader.h"
#include <filesystem>
#include <nlohmann/json.hpp>

namespace fs = std::filesystem;

FieldData::FieldData(nlohmann::json j)
{
    _fieldId = j["FieldId"].get<int32>();
    _mapName = j["MapName"].get<std::string>();

    if(j.contains("PlayerStart"))
    {
        for (const auto& item : j["PlayerStart"])
        {
            _playerStarts.push_back(item["Position"].get<Vector3>());
        }
    }

    if(j.contains("FieldPortal"))
    {
        for (const auto& item : j["FieldPortal"])
        {
            _fieldsPortals.push_back(item["Position"].get<Vector3>());
        }
    }

    fs::path path = fs::current_path() / "Resources" / "Fields" / "NavMesh" / (_mapName + ".bin");
    _navMesh = NavMeshLoader::LoadNavMeshFromBin(path.string().c_str());
}

FieldData::~FieldData()
{
    dtFreeNavMesh(_navMesh);
}