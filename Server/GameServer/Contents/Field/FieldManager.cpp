#include "pch.h"
#include "FieldManager.h"

#include <filesystem>

#include "Field.h"
#include "Util/NavMeshLoader.h"
#include "LogManager.h"
#include <nlohmann/json.hpp>

namespace fs = std::filesystem;

FieldManager& GFieldManager = FieldManager::Instance();

void FieldManager::Init()
{
    Load();
}

void FieldManager::Load()
{
    fs::path path = std::filesystem::current_path() / "Resources" / "Fields";
    for (const auto& entry : std::filesystem::directory_iterator(path))
    {
        if(entry.path().extension().string() != ".json") continue;

        std::ifstream file(entry.path());
        nlohmann::json json;
        file >> json;

        auto fieldData = make_shared<FieldData>(json);
        auto fieldId = fieldData->FieldId();
        _fieldDatas[fieldId] = std::move(fieldData);
    }
}

void FieldManager::Create(int32 fieldId)
{
    static int32 instance = 0;
    {
        WRITE_LOCK;
        auto fieldIt = _fieldDatas.find(fieldId);
        if (fieldIt == _fieldDatas.end() || fieldIt->second == nullptr) return;
        auto fieldData = fieldIt->second;

        //uint64 id = (fieldId << 32) | instance;

        _fields[fieldId] = make_shared<Field>(fieldId, fieldData.get());
        _fields[fieldId]->Init();
    }

    LOG_INFO(Default, "FieldManager : {} is Created", fieldId);
}

void FieldManager::Destroy(uint64 fieldId)
{
    WRITE_LOCK;
    _fields.erase(fieldId);
}

shared_ptr<Field> FieldManager::GetField(uint16 fieldId)
{
    shared_ptr<Field> field = nullptr;
    {
        READ_LOCK;
        auto fieldIt = _fields.find(fieldId);
        field = fieldIt == _fields.end() ? nullptr : fieldIt->second;
    }

    if (field == nullptr)
    {
        Create(fieldId);
    }

    field = _fields[fieldId];
    return field;
}
