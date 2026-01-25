#pragma once

#include "Util\NavMeshLoader.h"

class Field;
class FieldManager;
class PlayerCharacter;

extern FieldManager& GFieldManager;
class FieldManager : public Singleton<FieldManager>
{
public:
	void Create(uint32 id)
	{
		auto field = Find(id);
		if (field != nullptr) return;

		_fields.emplace(id, make_shared<Field>(id));
	}
	
	shared_ptr<Field> Find(uint32 id) 
	{
		if (_fields.find(id) == _fields.end()) return nullptr;
		return _fields[id];
	}

private:
	unordered_map<uint32, shared_ptr<Field>> _fields;
};

class Field : public enable_shared_from_this<Field>
{
public:
	Field(uint32 id) : _id(id) {}

	void EnterPlayer(shared_ptr<PlayerCharacter>);
	void BroadCast(SendBufferRef sendBuffer, const shared_ptr<PlayerCharacter>& except = nullptr);

private:
	USE_LOCK;
	std::vector<shared_ptr<PlayerCharacter>> _players;

	uint32 _id;
	//dtNavMesh* _navMesh;
};

