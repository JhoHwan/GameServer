#pragma once
#include <nlohmann/json_fwd.hpp>
#include "FieldData.h"

class dtNavMesh;
class FieldManager;
extern FieldManager& GFieldManager;

class FieldManager : public Singleton<FieldManager>
{
    friend class Field;
public:
    void Init();
    shared_ptr<class Field> GetField(uint16 mapId);

    const FieldData* GetFieldData(uint16 mapId)
    {
        READ_LOCK;
        auto it = _fieldDatas.find(mapId);
        if(it == _fieldDatas.end()) return nullptr;
        return (it->second).get();
    }

private:
    void LoadFieldDatas();

    shared_ptr<Field> Create(uint16 mapId);
    void Destroy(uint64 fieldId);

    static uint64 MakeFieldID(uint16 mapId, uint64 instanceId);

private:
    USE_LOCK;
    atomic<uint32> _instanceIDGenerator {1};

    unordered_map<uint64, shared_ptr<Field>> _fieldIdInstanceMap;
    unordered_map<uint16, unordered_set<shared_ptr<Field>>> _fields;

    unordered_map<uint16, unique_ptr<FieldData>> _fieldDatas;
    };