#include "pch.h"
#include "GameSession.h"
#include "Packet\ServerPacketHandler.h"

GameSession::GameSession() : _jobQueue(make_shared<JobQueue>())
{
}

void GameSession::OnRecvPacket(BYTE* buffer, int32 len)
{
	auto sessionRef = GetSessionRef();
	ServerPacketHandler::HandlePacket(sessionRef, buffer, len);
}

void GameSession::OnConnected()
{
	//cout << "Connect" << endl;
	SetTimeOut(5000, L"Login Request");
}

void GameSession::SetTimeOut(uint64 time, wstring log)
{
	JobRef job = make_shared<Job>([self = GetSessionRef(), log]
		{
			wcout << L"[Timeout] "<< log << " Timeout!" << endl;
			self->Disconnect(L"Time Out");
		});
	_timeOutToken = GJobTimer.Reserve(time, GetJobQueue(), job);
}
