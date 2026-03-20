#pragma once
#include "DetourNavMesh.h"
#include "../GameObject.h"
#include "Contents/Component.h"
#include "Util/Vector3.h"

class FieldData;
class Field;
class FieldManager;
class PlayerCharacter;
class dtNavMeshQuery;



class Field : public AsyncActor, public std::enable_shared_from_this<Field>
{
public:

	Field(uint64 id, const FieldData* fieldData);
	~Field() override;

	void Init();
	void BroadCast(SendBufferRef sendBuffer, const shared_ptr<PlayerCharacter>& except = nullptr);

	void UpdatePlayerPosition();

public:
	template <GameObjectType T, typename... Args>
	void Spawn(const Vector3& spawnPos, Args&&... args);
	void Despawn(uint64 id);

public:
	uint16 GetMapID() const { return static_cast<uint16>(_id >> 48); }
	uint64 GetID() const { return _id; }
	uint64 GetInstanceID() const
	{ return (_id & 0x0000FFFFFFFFFFFFULL); }
	const std::unordered_set<shared_ptr<PlayerCharacter>>& GetPlayers() const { return _players; }

	uint32 GetPlayerCount() const {return _currentPlayerCount.load(); }
	bool CanEnterField() const {return GetPlayerCount() < MAX_PLAYERS; }

	void HandleRequestUsePortal(const weak_ptr<PlayerCharacter>& playerRef, uint32 portalId);
	void HandleRequestMove(const weak_ptr<PlayerCharacter>& playerRef, const Vector3& dest, const uint64& startServerTick);

	// NOT-THREAD-SAFE
	void EnterPlayer(const shared_ptr<PlayerCharacter>& player);
	void LeavePlayer(const shared_ptr<PlayerCharacter>& player);

	void PreEnter() {_currentPlayerCount.fetch_add(1);}

private:
	void FindPath(const Vector3& pos, const Vector3& endPos, OUT std::vector<Vector3>& result);

private:
	unordered_map<uint64, shared_ptr<GameObject>> _objects;
	std::unordered_set<shared_ptr<PlayerCharacter>> _players;

	const uint32 MAX_PLAYERS = 36;

	atomic<uint32> _currentPlayerCount {0};
	atomic<uint64> _destroyToken{0};

	uint64 _id;
	dtNavMesh* _navMesh;
	dtNavMeshQuery* _navQuery;

private:
	const FieldData* const _fieldData;
};

template<GameObjectType T, typename ... Args>
void Field::Spawn(const Vector3& spawnPos, Args&&... args)
{
	shared_ptr<T> newObject = GameObject::Create<T>(std::forward<Args>(args)...);
	newObject->SetField(shared_from_this());
	newObject->Transform()->SetPos(spawnPos);

	weak_ptr<Field> fieldRef = weak_from_this();

	DoAsync([fieldRef = std::move(fieldRef), newObject = std::move(newObject)]() {
		auto self = fieldRef.lock();
		if (!self) return;

		self->_objects.emplace(newObject->GetId(), newObject);
		newObject->OnSpawn();
	});
}

