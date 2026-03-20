#pragma once
#include <utility>

#include "GameObject.h"
#include "JobQueue.h"
#include "LogManager.h"

class GameSession;

class PlayerCharacter : public GameObject
{
public:
	constexpr static uint32 TAG = MakeTag(EObjectType::Player, 1);

public:
	PlayerCharacter(weak_ptr<GameSession> session);
	~PlayerCharacter() override;

public:
	void Init() override;
	void OnSpawn() override;
	void OnDespawn() override;

public:
	void HandleMoveRequest(const Protocol::Vector3& dest, uint64 startServerTick);

public:
	void SetMoveInfo(vector<Vector3> wayPoints, vector<uint64> moveArrivalTime, uint64 moveStartTime);
	const vector<Vector3>& GetWaypoints() const { return _moveWaypoints; }
	float GetMoveSpeed() const { return _moveSpeed; }

	const uint64& GetMoveStartTime() const { return _moveStartTime; }
	Vector3 GetCurrentPosition(uint64 now) const;
	Vector3 GetDestinationPosition() const { return _moveWaypoints.back(); }

	bool IsMoving() const{return _isMoving;}

	const vector<uint64>& GetArrivalTimes() const { return _moveArrivalTimes; }

public:
	shared_ptr<GameSession> GetSession() const { return _sessionRef.lock(); }
	shared_ptr<PlayerCharacter> GetPlayerRef() {return static_pointer_cast<PlayerCharacter>(shared_from_this()); }

private:
	weak_ptr<GameSession> _sessionRef;
	std::vector<Vector3> _moveWaypoints;
	std::vector<uint64> _moveArrivalTimes;

	uint64 _moveStartTime = 0;
	float _moveSpeed = 500.0f; // 임시 속도
	bool _isMoving = false;
};

