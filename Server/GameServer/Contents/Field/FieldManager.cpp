#include "pch.h"
#include "FieldManager.h"

#include <filesystem>

#include "Field.h"
#include "Util/NavMeshLoader.h"
#include "LogManager.h"
#include <nlohmann/json.hpp>

#include "Util/MonitorManager.h"

namespace fs = std::filesystem;

FieldManager& GFieldManager = FieldManager::Instance();

void FieldManager::Init()
{
    LoadFieldDatas();
}

void FieldManager::LoadFieldDatas()
{
    fs::path path = std::filesystem::current_path() / "Resources" / "Fields";
    for (const auto& entry : std::filesystem::directory_iterator(path))
    {
        if(entry.path().extension().string() != ".json") continue;

        std::ifstream file(entry.path());
        nlohmann::json json;
        file >> json;

        unique_ptr<FieldData> fieldData = std::make_unique<FieldData>(json);
        uint16 mapId = fieldData->MapId;
        if(_fieldDatas.contains(mapId))
        {
            LOG_ERROR(FieldManager, "Duplicate IDs exist in the field data.")
            continue;
        }

        _fieldDatas.emplace(mapId, std::move(fieldData));
    }
}

shared_ptr<Field> FieldManager::Create(uint16 mapid)
{
    shared_ptr<Field> field = nullptr;
    uint16 instanceID = _instanceIDGenerator.fetch_add(1);
    uint64 fieldId = MakeFieldID(mapid, instanceID);

    auto fieldIt = _fieldDatas.find(mapid);
    if (fieldIt == _fieldDatas.end()) return nullptr;
    const FieldData* const fieldData = fieldIt->second.get();

    field = make_shared<Field>(fieldId, fieldData);
    field->Init();
    _fields[mapid].insert(field);
    _fieldIdInstanceMap[fieldId] = field;

    field->PreEnter();
    GMonitorManager.AddFieldCount();
    LOG_INFO(Default, "FieldManager : {}{} is Created", field->GetMapID(), field->GetInstanceID());
    return field;
}

void FieldManager::Destroy(uint64 fieldId)
{
    WRITE_LOCK;
    auto it = _fieldIdInstanceMap.find(fieldId);
    if(it == _fieldIdInstanceMap.end())
    {
        return;
    }

    shared_ptr<Field> field = it->second;
    _fieldIdInstanceMap.erase(fieldId);

    uint16 mapId = field->GetMapID();

    auto fieldIt = _fields.find(mapId);
    if(fieldIt == _fields.end())
    {
        return;
    }

    GMonitorManager.ReleaseFieldCount();

    _fields[mapId].erase(field);
    if(_fields[mapId].empty())
    {
        _fields.erase(mapId);
    }
}

shared_ptr<Field> FieldManager::GetField(uint16 mapId)
{
    WRITE_LOCK;
    auto it = _fields.find(mapId);
    if(it != _fields.end())
    {
        for(const auto& field : it->second)
        {
            if(field->CanEnterField())
            {
                field->PreEnter();
                return field;
            }
        }
    }
    return Create(mapId);
}

uint64 FieldManager::MakeFieldID(uint16 mapId, uint64 instanceId)
{
    return static_cast<uint64>(mapId) << 48 | instanceId;
}
