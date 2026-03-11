#include "pch.h"
#include "Player.h"
#include "GameSession.h"
#include "LogManager.h"

PlayerCharacter::PlayerCharacter(weak_ptr<GameSession> session) : _sessionRef(session)
{
	uint16 objectTag = MakeTag(EObjectType::Player, 1);
	SetId(objectTag);
}

PlayerCharacter::~PlayerCharacter()
{
	LOG_DEBUG(Default, "PlayerCharacter::~PlayerCharacter()");
}

void PlayerCharacter::Init()
{
	GameObject::Init();

	LOG_DEBUG(Default, "Player[{}] Created", GetInstanceID());

	auto session = GetSession();
	auto player = static_pointer_cast<PlayerCharacter>(shared_from_this());
	if (session) session->SetPlayer(player);
}

void PlayerCharacter::SetMoveInfo(std::vector<Vector3> waypoints, uint64 startTime, float speed)
{
	if(waypoints.empty()) return;

	_moveWaypoints = std::move(waypoints);

	_moveArrivalTimes.clear();
	_moveArrivalTimes.reserve(_moveWaypoints.size());
	_moveArrivalTimes.push_back(startTime);

	_moveStartTime = startTime;
	_moveSpeed = speed;
	_isMoving = true;

	uint64 accumulatedTime = startTime;
	float totalDist = 0;
	for(int i = 1; i < _moveWaypoints.size(); i++)
	{
		float dist = Vector3::Dist2D(_moveWaypoints[i-1], _moveWaypoints[i]);
		totalDist += dist;

		float seconds = dist / speed;

		uint64 timeToTravel = static_cast<uint64>(seconds * 1000.0f);
		accumulatedTime += timeToTravel;
		_moveArrivalTimes.push_back(accumulatedTime);
	}

	JobRef job = make_shared<Job>([weakGameObject = weak_from_this(), moveToken = _moveStartTime]()
	{
		auto gameObject = weakGameObject.lock();
		if (!gameObject) return;

		auto self = static_pointer_cast<PlayerCharacter>(gameObject);
		if(moveToken != self->_moveStartTime) return;

		auto arrivalPos = self->_moveWaypoints.back();
		LOG_INFO(PathFind, "Player{} Arrive [{}, {}, {}]", self->GetId(), arrivalPos.x, arrivalPos.y, arrivalPos.z);

		if(self->_isMoving)
		{
			self->_isMoving = false;
			self->Transform()->SetPos(arrivalPos);
		}
	});

	LOG_DEBUG(PathFind, "Arrive at {} ({}ms)", accumulatedTime, accumulatedTime - _moveStartTime);
	LJobTimer.Reserve(accumulatedTime - _moveStartTime, GetJobQueue(), job);
}

Vector3 PlayerCharacter::GetCurrentPosition(uint64 now) const
{
	if(!_isMoving || _moveWaypoints.empty() || now <= _moveStartTime) return Transform()->GetPos();
	if(now >= _moveArrivalTimes.back()) return _moveWaypoints.back();

	for (size_t i = 1; i < _moveArrivalTimes.size(); ++i)
	{
		if (now <= _moveArrivalTimes[i])
		{
			uint64 segmentStartTime = _moveArrivalTimes[i - 1];
			uint64 segmentEndTime = _moveArrivalTimes[i];

			float ratio = 0.0f;
			if (segmentEndTime > segmentStartTime)
			{
				ratio = static_cast<float>(now - segmentStartTime) / static_cast<float>(segmentEndTime - segmentStartTime);
			}

			Vector3 start = _moveWaypoints[i - 1];
			Vector3 end = _moveWaypoints[i];

			Vector3 diff = end - start;
			Vector3 currentPos = start + (diff * ratio);
			//LOG_DEBUG(Default, "Player Position Update [{}, {}, {}]", currentPos.x, currentPos.y, currentPos.z);
			return currentPos;
		}
	}

	Vector3 finalPos = _moveWaypoints.back();
	return finalPos;
}
