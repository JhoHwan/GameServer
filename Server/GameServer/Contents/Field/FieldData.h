#pragma once

#include "nlohmann/json.hpp"
#include "Util/Vector3.h"

class dtNavMesh;

class FieldData
{
public:
    FieldData(nlohmann::json j);
    ~FieldData();

    int32 FieldId() const { return _fieldId; }
    dtNavMesh* NavMesh() const { return _navMesh; }
    const std::vector<Vector3>& PlayerStarts() const { return _playerStarts; }
    const std::vector<Vector3>& FieldsPortals() const { return _fieldsPortals; }
    const std::string& MapName() const { return _mapName; }

private:
    int32 _fieldId;
    dtNavMesh* _navMesh;
    std::string _mapName;
    std::vector<Vector3> _playerStarts;
    std::vector<Vector3> _fieldsPortals;
};