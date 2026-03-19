#include "pch.h"
#include "Player.h"

#include <utility>
#include "GameSession.h"
#include "LogManager.h"
#include "Field/Field.h"
#include "Util/Time.h"

PlayerCharacter::PlayerCharacter(weak_ptr<GameSession> session) : _sessionRef(std::move(session))
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

void PlayerCharacter::HandleMoveRequest(const Protocol::Vector3& dest, uint64 startServerTick)
{
	constexpr int32 MOVE_REQUEST_MIN_INTERVAL = 500;
	constexpr float MOVE_REQUEST_MIN_DIST = 300.0f;

	auto& time = GetMoveStartTime();
	if(IsMoving() && startServerTick - time < MOVE_REQUEST_MIN_INTERVAL)
	{
		if(Vector3::Dist2D(dest, GetDestinationPosition()) <= MOVE_REQUEST_MIN_DIST)
		{
			return;
		}
	}

	GetField()->HandleRequestMove(GetPlayerRef(), dest, startServerTick);
}

void PlayerCharacter::SetMoveInfo(vector<Vector3> wayPoints, vector<uint64> moveArrivalTime, uint64 moveStartTime)
{
	DoAsync([self = GetPlayerRef(), wayPoints = std::move(wayPoints), moveArrivalTime = std::move(moveArrivalTime), moveStartTime]()
	{
		self->_moveWaypoints = wayPoints;
		self->_moveStartTime = moveStartTime;
		self->_isMoving = true;
		self->_moveArrivalTimes = moveArrivalTime;

		JobRef job = make_shared<Job>([weakGameObject = self->weak_from_this(), moveToken = self->_moveStartTime]()
		{
			auto gameObject = weakGameObject.lock();
			if (!gameObject) return;

			auto self = static_pointer_cast<PlayerCharacter>(gameObject);
			if(moveToken != self->_moveStartTime) return;

			auto arrivalPos = self->_moveWaypoints.back();
			LOG_INFO(PathFind, "Player {}{} Arrive [{}, {}, {}]", self->GetSubID(), self->GetInstanceID(), arrivalPos.x, arrivalPos.y, arrivalPos.z);

			if(self->_isMoving)
			{
				self->_isMoving = false;
				self->Transform()->SetPos(arrivalPos);
			}
		});

		LJobTimer.Reserve(self->_moveArrivalTimes.back() - moveStartTime, self->GetJobQueue(), job);
	});
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
			Transform()->SetPos(currentPos);
			return currentPos;
		}
	}

	Vector3 finalPos = _moveWaypoints.back();
	return finalPos;
}
