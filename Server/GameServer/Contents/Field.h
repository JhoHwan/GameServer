#pragma once

#include "Util/NavMeshLoader.h"

class FieldInstance;
class FieldManager;
class PlayerCharacter;

extern FieldManager& GFieldManager;
class FieldManager : public Singleton<FieldManager>
{
public:
	void Init();

	void Create(uint16 fieldId);

	shared_ptr<FieldInstance> GetField(uint16 fieldId);

private:
	USE_LOCK;
	unordered_map<uint16, shared_ptr<FieldInstance>> _fields;
	unordered_map<uint16, dtNavMesh*> _navMesh;
};

class FieldInstance : public JobQueue
{
public:
	FieldInstance(uint16 id, dtNavMesh* navMesh);

	void EnterPlayer(shared_ptr<PlayerCharacter> );
	void BroadCast(SendBufferRef sendBuffer, const shared_ptr<PlayerCharacter>& except = nullptr);
	shared_ptr<FieldInstance> GetFieldRef() {return static_pointer_cast<FieldInstance>(shared_from_this());}
private:
	std::vector<shared_ptr<PlayerCharacter>> _players;

	uint16 _id;
	dtNavMesh* _navMesh;
	dtNavMeshQuery* _navQuery;
};

