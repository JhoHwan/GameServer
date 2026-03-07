#include "pch.h"
#include "GameSession.h"

#include "LogManager.h"
#include "Packet/ServerPacketHandler.h"

GameSession::GameSession() : _jobQueue(make_shared<JobQueue>()), _timeOutToken(0)
{
}

void GameSession::OnRecvPacket(BYTE* buffer, int32 len)
{
	auto sessionRef = GetSessionRef();
	ServerPacketHandler::HandlePacket(sessionRef, buffer, len);
}

void GameSession::OnConnected()
{
	LOG_INFO("Client Connected : {}", GetAddress().GetIpAddress());
	SetTimeOut(5000, "Login Request");
}

void GameSession::OnDisconnected()
{
	PacketSession::OnDisconnected();

	_playerRef.reset();
}

void GameSession::SetTimeOut(uint64 time, const string& log)
{
	weak_ptr<Session> self = GetSessionRef();
	JobRef job = make_shared<Job>([this, self, log, token = _timeOutToken.load()]
		{
			SessionRef session = self.lock();
			if (!session || _timeOutToken.load() != token) return;
			cout << "[Timeout] " << log << " Timeout!" << endl;
			session->Disconnect("Time Out");
		});
	LJobTimer.Reserve(time, GetJobQueue(), job);
}

void GameSession::CancelTimeOut()
{
	_timeOutToken.fetch_add(1);
}
