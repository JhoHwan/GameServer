#pragma once
#include <utility>

#include "GameObject.h"
#include "JobQueue.h"
#include "LogManager.h"

class GameSession;

class PlayerCharacter : public GameObject, public AsyncActor
{
public:
	PlayerCharacter(weak_ptr<GameSession> session);
	~PlayerCharacter() override;

public:
	void Init() override;
	void SetMoveInfo(vector<Vector3> waypoints, uint64 startTime, float speed);

	Vector3 GetCurrentPosition(uint64 now) const;
public:
	shared_ptr<GameSession> GetSession() const { return _sessionRef.lock(); }

	bool IsMoving() const{return _isMoving;}

private:
	weak_ptr<GameSession> _sessionRef;

	std::vector<Vector3> _moveWaypoints;
	std::vector<uint64> _moveArrivalTimes;

	uint64 _moveStartTime = 0;
	float _moveSpeed = 500.0f; // 임시 속도
	bool _isMoving = false;
};

