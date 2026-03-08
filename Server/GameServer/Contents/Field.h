#pragma once

#include "Protocol.pb.h"
#include "Util/NavMeshLoader.h"
#include "Util/Vector3.h"

class FieldInstance;
class FieldManager;
class PlayerCharacter;

extern FieldManager& GFieldManager;
class FieldManager : public Singleton<FieldManager>
{
public:
	void Init();

	void Create(uint16 fieldId);
	void Destroy(uint16 fieldId);

	shared_ptr<FieldInstance> GetField(uint16 fieldId);

private:
	USE_LOCK;
	unordered_map<uint16, shared_ptr<FieldInstance>> _fields;
	unordered_map<uint16, dtNavMesh*> _navMesh;
};

class FieldInstance : public AsyncActor, public std::enable_shared_from_this<FieldInstance>
{
public:
	FieldInstance(uint16 id, dtNavMesh* navMesh);
	~FieldInstance();

	void Init();

	void EnterPlayer(shared_ptr<PlayerCharacter> );
	void BroadCast(SendBufferRef sendBuffer, const shared_ptr<PlayerCharacter>& except = nullptr);

	void PlayerRequestMove(weak_ptr<PlayerCharacter> player, const Protocol::Vector3& pos);

	void LeavePlayer(std::shared_ptr<PlayerCharacter> player, shared_ptr<FieldInstance> nextField = nullptr);

	void UpdatePlayerPosition();

private:
	void FindPath(const Vector3& pos, const Vector3& endPos, OUT std::vector<Vector3>& result);

private:
	std::unordered_set<shared_ptr<PlayerCharacter>> _players;

	uint16 _id;
	dtNavMesh* _navMesh;
	dtNavMeshQuery* _navQuery;
};

