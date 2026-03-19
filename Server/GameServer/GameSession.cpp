#include "pch.h"
#include "GameSession.h"

#include "LogManager.h"
#include "Protocol.pb.h"
#include "Service.h"
#include "Contents/Field/Field.h"
#include "Contents/Player.h"
#include "Packet/ServerPacketHandler.h"
#include "Util/MonitorManager.h"
#include "Util/Time.h"

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
	_lastPongTime.store(Time::GetServerTime());
	SendPing();
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

void GameSession::SendPing()
{
	auto lastPongTime = _lastPongTime.load();
	auto now = Time::GetServerTime();

	if(now - lastPongTime > 20000)
	{
		LOG_DEBUG(Timeout, "HeartBeat Timeout!");
		Disconnect("HeartBeat Time Out");
		return;
	}

	Protocol::SC_PING pkt;
	pkt.set_server_send_tick(Time::GetServerTime());
	SendPacket(ServerPacketHandler::MakeSendBuffer(pkt));

	const int32 interval = _syncPingsReceived.load() < INITIAL_PING_COUNT ? INITIAL_PING_INTERVAL : DEFAULT_PING_INTERVAL;
	weak_ptr sessionRef = static_pointer_cast<GameSession>(shared_from_this());
	JobRef job = make_shared<Job>([sessionRef]
	{
		auto self = sessionRef.lock();
		if(!self) return;
		self->SendPing();
	});

	LJobTimer.Reserve(interval, GetJobQueue(), job);
}

void GameSession::HandlePong(const uint64& T1, const uint64& T2, const uint64& T3)
{
	_lastPongTime.store(Time::GetServerTime());
	auto T4 = Time::GetServerTime();

	uint64 rtt = (T4 - T1) - (T3 - T2);
	uint64 latency = rtt / 2;
	int64 clientTimeAtT4 = T3 + latency;
	int64 currentOffset = static_cast<int64>(T4) - static_cast<int64>(clientTimeAtT4);

	auto syncCount =_syncPingsReceived.fetch_add(1);
	if(syncCount < INITIAL_PING_COUNT)
	{
		_accumulatedOffset.fetch_add(currentOffset);
		syncCount++;
		if(syncCount == INITIAL_PING_COUNT)
		{
			auto avgOffset = _accumulatedOffset.load() / INITIAL_PING_COUNT;
			_serverOffset.store(avgOffset);

			Protocol::SC_TIME_SYNC syncPkt;
			syncPkt.set_server_offset(avgOffset);
			syncPkt.set_rtt(rtt);
			SendPacket(ServerPacketHandler::MakeSendBuffer(syncPkt));


		}
		return;
	}

	int64 prevOffset = _serverOffset.load();
	auto smoothedOffset = static_cast<int64>((prevOffset * 0.9f) + (currentOffset * 0.1f));
	_serverOffset.store(smoothedOffset);
	_prevLatency.store(latency);
	Protocol::SC_TIME_SYNC syncPkt;
	syncPkt.set_server_offset(smoothedOffset);
	syncPkt.set_rtt(rtt);
	SendPacket(ServerPacketHandler::MakeSendBuffer(syncPkt));
}
