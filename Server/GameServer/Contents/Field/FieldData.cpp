#include "pch.h"
#include "FieldData.h"
#include "Util/NavMeshLoader.h"
#include <filesystem>
#include <nlohmann/json.hpp>

namespace fs = std::filesystem;

FieldData::FieldData(const nlohmann::json& j)
{
    MapId = j["MapId"].get<uint16>();
    MapName = j["MapName"].get<std::string>();

    if(j.contains("PlayerStarts"))
    {
        for (const auto& item : j["PlayerStarts"])
        {
            PlayerStarts.push_back(item["Position"].get<Vector3>());
        }
    }

    if(j.contains("FieldPortals"))
    {
        for (const auto& item : j["FieldPortals"])
        {
            FieldsPortals.push_back(item.get<FieldPortalData>());
        }
    }

    fs::path path = fs::current_path() / "Resources" / "Fields" / "NavMesh" / (MapName + ".bin");
    NavMesh = NavMeshLoader::LoadNavMeshFromBin(path.string().c_str());
}

FieldData::~FieldData()
{
    dtFreeNavMesh(NavMesh);
}