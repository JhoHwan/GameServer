#pragma once

#include "nlohmann/json_fwd.hpp"
#include "Util/Vector3.h"

struct FieldPortalData;
class dtNavMesh;

class FieldData
{
public:
    FieldData(const nlohmann::json& j);
    ~FieldData();

    uint16 MapId;
    dtNavMesh* NavMesh;
    std::string MapName;
    std::vector<Vector3> PlayerStarts;
    std::vector<FieldPortalData> FieldsPortals;
};

struct FieldPortalData
{
    uint8 PortalId;
    Vector3 Position;
    uint16 TargetMapId;
    uint8 TargetPortalId;
};

inline void from_json(const nlohmann::json& j, FieldPortalData& f)
{
    j.at("PortalId").get_to(f.PortalId);
    j.at("Position").get_to(f.Position);
    j.at("TargetMapId").get_to(f.TargetMapId);
    j.at("TargetPortalId").get_to(f.TargetPortalId);
}