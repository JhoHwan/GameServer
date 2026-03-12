#pragma once

#include "nlohmann/json.hpp"
#include "Util/Vector3.h"

class dtNavMesh;

class FieldData
{
public:
    FieldData(nlohmann::json j);
    ~FieldData();

    uint16 FieldId() const { return _fieldId; }
    dtNavMesh* NavMesh() const { return _navMesh; }
    const std::vector<Vector3>& PlayerStarts() const { return _playerStarts; }
    const std::vector<Vector3>& FieldsPortals() const { return _fieldsPortals; }
    const std::string& MapName() const { return _mapName; }

private:
    uint16 _fieldId;
    dtNavMesh* _navMesh;
    std::string _mapName;
    std::vector<Vector3> _playerStarts;
    std::vector<Vector3> _fieldsPortals;
};