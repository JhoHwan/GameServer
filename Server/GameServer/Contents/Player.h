#pragma once
#include <utility>

#include "GameObject.h"
#include "JobQueue.h"

class GameSession;

class PlayerCharacter : public GameObject, public AsyncActor
{
public:
	PlayerCharacter(weak_ptr<GameSession> session) : _sessionRef(std::move(session))
	{

	}

public:
	void Init() override;
	void SetMoveInfo(const std::vector<Vector3>& waypoints, uint64 startTime, float speed)
	{
		_moveWaypoints = waypoints;
		_moveStartTime = startTime;
		_moveSpeed = speed;
		_isMoving = true;
	}

	Vector3 GetCurrentPosition(uint64 now);
public:
	shared_ptr<GameSession> GetSession() const { return _sessionRef.lock(); }

private:
	weak_ptr<GameSession> _sessionRef;

	std::vector<Vector3> _moveWaypoints;
	uint64 _moveStartTime = 0;
	float _moveSpeed = 500.0f; // 임시 속도
	bool _isMoving = false;
};

