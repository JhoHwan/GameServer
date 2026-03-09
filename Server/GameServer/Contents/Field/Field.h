#pragma once

#include "Util/NavMeshLoader.h"
#include "Util/Vector3.h"

class Field;
class FieldManager;
class PlayerCharacter;


class Field : public AsyncActor, public std::enable_shared_from_this<Field>
{
public:
	Field(uint16 id, dtNavMesh* navMesh);
	~Field();

	void Init();

	void EnterPlayer(shared_ptr<PlayerCharacter> );
	void BroadCast(SendBufferRef sendBuffer, const shared_ptr<PlayerCharacter>& except = nullptr);

	void PlayerRequestMove(weak_ptr<PlayerCharacter> player, const Protocol::Vector3& pos);

	void LeavePlayer(std::shared_ptr<PlayerCharacter> player, shared_ptr<Field> nextField = nullptr);

	void UpdatePlayerPosition();

private:
	void FindPath(const Vector3& pos, const Vector3& endPos, OUT std::vector<Vector3>& result);

private:
	std::unordered_set<shared_ptr<PlayerCharacter>> _players;

	uint16 _id;
	dtNavMesh* _navMesh;
	class dtNavMeshQuery* _navQuery;
};

