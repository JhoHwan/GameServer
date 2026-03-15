#pragma once
#include "DetourNavMesh.h"
#include "Protocol.pb.h"
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

	void PreEnter() { _currentPlayerCount.fetch_add(1); }
	void EnterPlayer(weak_ptr<PlayerCharacter> player);
	void BroadCast(SendBufferRef sendBuffer, const shared_ptr<PlayerCharacter>& except = nullptr);

	void PlayerRequestMove(weak_ptr<PlayerCharacter> player, const Protocol::Vector3& pos);

	void LeavePlayer(std::shared_ptr<PlayerCharacter> player);

	void UpdatePlayerPosition();

public:
	uint16 GetMapID() const
	{
		return static_cast<uint16>(_id >> 48);
	}

	uint64 GetID() const { return _id; }
	uint64 GetInstanceID() const
	{
		return (_id & 0x0000FFFFFFFFFFFFULL);
	}

	uint32 GetPlayerCount() const {return _currentPlayerCount.load(); }
	bool CanEnterField() const {return GetPlayerCount() < MAX_PLAYERS; }

	void RequestUsePortal(const weak_ptr<PlayerCharacter>& playerRef, uint32 portalId);

private:
	void FindPath(const Vector3& pos, const Vector3& endPos, OUT std::vector<Vector3>& result);

private:
	std::unordered_set<shared_ptr<PlayerCharacter>> _players;

	const uint32 MAX_PLAYERS = 36;

	atomic<uint32> _currentPlayerCount {0};
	atomic<uint64> _destroyToken{0};

	uint64 _id;
	dtNavMesh* _navMesh;
	dtNavMeshQuery* _navQuery;

	const FieldData* const _fieldData;
};

