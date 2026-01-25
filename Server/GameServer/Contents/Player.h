#pragma once
#include "GameObject.h"

class GameSession;

class PlayerCharacter : public GameObject
{
public:
	PlayerCharacter(weak_ptr<GameSession> session) : _sessionRef(session), _jobQueue(make_shared<JobQueue>())
	{

	}

public:
	virtual void Init() override;

public:
	//void SetSession(weak_ptr<GameSession> session) { _sessionRef = session; }
	shared_ptr<GameSession> GetSession() const { return _sessionRef.lock(); }
	shared_ptr<JobQueue> GetJobQueue() { return _jobQueue; }

private:
	weak_ptr<GameSession> _sessionRef;
	shared_ptr<JobQueue> _jobQueue;
};

