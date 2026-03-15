#include "pch.h"
#include "GameSession.h"

#include "LogManager.h"
#include "Contents/Field/Field.h"
#include "Contents/Player.h"
#include "Packet/ServerPacketHandler.h"
#include "Util/MonitorManager.h"

GameSession::GameSession() : _jobQueue(make_shared<JobQueue>()), _timeOutToken(0)
{
}

GameSession::~GameSession()
{
	LOG_DEBUG(Default, "GameSession::~GameSession()");
}

void GameSession::OnRecvPacket(BYTE* buffer, int32 len)
{
	GMonitorManager.AddInPacket();
	auto sessionRef = GetSessionRef();
	ServerPacketHandler::HandlePacket(sessionRef, buffer, len);
}

void GameSession::OnConnected()
{
	GMonitorManager.AddCCU();

	//LOG_INFO(Default, "Client Connected : {}", GetAddress().GetIpAddress());
	SetTimeOut(30000, "Login Request");
}

void GameSession::OnDisconnected()
{
	PacketSession::OnDisconnected();

	GMonitorManager.ReleaseCCU();

	LOG_DEBUG(Default, "Client DisConnected : {}", GetAddress().GetIpAddress());

	if(_playerRef)
	{
		if(_playerRef->GetField())
		{
			_playerRef->GetField()->LeavePlayer(_playerRef);
		}
	}
	_playerRef.reset();
}

void GameSession::SetTimeOut(uint64 time, const string& log)
{
	uint64 token = _timeOutToken.fetch_add(1)+1;

	weak_ptr<GameSession> self = static_pointer_cast<GameSession>(GetSessionRef());
	JobRef job = make_shared<Job>([self, log, token]
		{
			shared_ptr<GameSession> session = self.lock();
			if(!session) return;

			auto curToken = session->_timeOutToken.load();
			if (curToken != token) return;


			LOG_DEBUG(Timeout, "{} Timeout!", log);
			session->Disconnect("Time Out");
		});
	LJobTimer.Reserve(time, GetJobQueue(), job);
}

void GameSession::CancelTimeOut()
{
	_timeOutToken.fetch_add(1);
}

void GameSession::SendPacket(const SendBufferRef& sendBuffer)
{
	GMonitorManager.AddOutPacket();
	Session::Send(sendBuffer);
}
