#pragma once
#include "Util/Vector3.h"
#include <nlohmann/json_fwd.hpp>
#include "FieldData.h"

class dtNavMesh;
class FieldManager;
extern FieldManager& GFieldManager;

class FieldManager : public Singleton<FieldManager>
{
public:
    void Init();
    void Load();

    void Create(uint64 fieldId);
    void Destroy(uint64 fieldId);

    shared_ptr<class Field> GetField(uint16 fieldId);

private:
    USE_LOCK;
    unordered_map<uint64, shared_ptr<Field>> _fields;
    unordered_map<int32, shared_ptr<FieldData>> _fieldDatas;
};