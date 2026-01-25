#pragma once
#include "Protocol.pb.h"

#if UE_BUILD_DEBUG + UE_BUILD_DEVELOPMENT + UE_BUILD_TEST + UE_BUILD_SHIPPING >= 1
using SessionRef = TSharedPtr<class FGNSession>;
using SendBufferRef = TSharedPtr<class FSendBuffer>;
#include "GameNet.h"
#else
using SessionRef = shared_ptr<class Session>;
using SendBufferRef = shared_ptr<class SendBuffer>;
#endif

using PacketHandlerFunc = std::function<bool(SessionRef&, BYTE*, int32)>;
extern PacketHandlerFunc GPacketHandler[UINT16_MAX];

enum : uint16
{
	PKT_CS_REQ_ENTER_GAME = 1000,
	PKT_SC_ENTER_GAME_RESULT = 1001,
	PKT_CS_REQ_MOVE_FIELD = 1002,
	PKT_SC_MOVE_FIELD_FAIL = 1003,
	PKT_SC_START_FIELD_LOADING = 1004,
	PKT_CS_FIELD_LOADING_COMPLETE = 1005,
	PKT_SC_ENTER_FIELD = 1006,
	PKT_SC_SPAWN_PLAYER = 1007,
	PKT_SC_DESPAWN_PLAYER = 1008,
	PKT_CS_MOVE = 1009,
	PKT_CS_PING = 1010,
	PKT_SC_PONG = 1011,
};

// Custom Handlers
bool Handle_INVALID(SessionRef& session, BYTE* buffer, int32 len);
bool Handle_CS_REQ_ENTER_GAME(SessionRef& session, Protocol::CS_REQ_ENTER_GAME& pkt);
bool Handle_CS_REQ_MOVE_FIELD(SessionRef& session, Protocol::CS_REQ_MOVE_FIELD& pkt);
bool Handle_CS_FIELD_LOADING_COMPLETE(SessionRef& session, Protocol::CS_FIELD_LOADING_COMPLETE& pkt);
bool Handle_CS_MOVE(SessionRef& session, Protocol::CS_MOVE& pkt);
bool Handle_CS_PING(SessionRef& session, Protocol::CS_PING& pkt);

class ServerPacketHandler
{
public:
	static void Init()
	{
		for (int32 i = 0; i < UINT16_MAX; i++)
			GPacketHandler[i] = Handle_INVALID;
		GPacketHandler[PKT_CS_REQ_ENTER_GAME] = [](SessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::CS_REQ_ENTER_GAME>(Handle_CS_REQ_ENTER_GAME, session, buffer, len); };
		GPacketHandler[PKT_CS_REQ_MOVE_FIELD] = [](SessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::CS_REQ_MOVE_FIELD>(Handle_CS_REQ_MOVE_FIELD, session, buffer, len); };
		GPacketHandler[PKT_CS_FIELD_LOADING_COMPLETE] = [](SessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::CS_FIELD_LOADING_COMPLETE>(Handle_CS_FIELD_LOADING_COMPLETE, session, buffer, len); };
		GPacketHandler[PKT_CS_MOVE] = [](SessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::CS_MOVE>(Handle_CS_MOVE, session, buffer, len); };
		GPacketHandler[PKT_CS_PING] = [](SessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::CS_PING>(Handle_CS_PING, session, buffer, len); };
	}

	static bool HandlePacket(SessionRef& session, BYTE* buffer, int32 len)
	{
		PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);
		return GPacketHandler[header->id](session, buffer, len);
	}
	static SendBufferRef MakeSendBuffer(Protocol::SC_ENTER_GAME_RESULT& pkt) { return MakeSendBuffer(pkt, PKT_SC_ENTER_GAME_RESULT); }
	static SendBufferRef MakeSendBuffer(Protocol::SC_MOVE_FIELD_FAIL& pkt) { return MakeSendBuffer(pkt, PKT_SC_MOVE_FIELD_FAIL); }
	static SendBufferRef MakeSendBuffer(Protocol::SC_START_FIELD_LOADING& pkt) { return MakeSendBuffer(pkt, PKT_SC_START_FIELD_LOADING); }
	static SendBufferRef MakeSendBuffer(Protocol::SC_ENTER_FIELD& pkt) { return MakeSendBuffer(pkt, PKT_SC_ENTER_FIELD); }
	static SendBufferRef MakeSendBuffer(Protocol::SC_SPAWN_PLAYER& pkt) { return MakeSendBuffer(pkt, PKT_SC_SPAWN_PLAYER); }
	static SendBufferRef MakeSendBuffer(Protocol::SC_DESPAWN_PLAYER& pkt) { return MakeSendBuffer(pkt, PKT_SC_DESPAWN_PLAYER); }
	static SendBufferRef MakeSendBuffer(Protocol::SC_PONG& pkt) { return MakeSendBuffer(pkt, PKT_SC_PONG); }

private:
	template<typename PacketType, typename ProcessFunc>
	static bool HandlePacket(ProcessFunc func, SessionRef& session, BYTE* buffer, int32 len)
	{
		PacketType pkt;
		if (pkt.ParseFromArray(buffer + sizeof(PacketHeader), len - sizeof(PacketHeader)) == false)
			return false;

		return func(session, pkt);
	}

	template<typename T>
	static SendBufferRef MakeSendBuffer(T& pkt, uint16 pktId)
	{
		const uint16 dataSize = static_cast<uint16>(pkt.ByteSizeLong());
		const uint16 packetSize = dataSize + sizeof(PacketHeader);

#if UE_BUILD_DEBUG + UE_BUILD_DEVELOPMENT + UE_BUILD_TEST + UE_BUILD_SHIPPING >= 1
		SendBufferRef sendBuffer = MakeShared<FSendBuffer>(packetSize);
#else
		SendBufferRef sendBuffer = make_shared<SendBuffer>(packetSize);
#endif
		PacketHeader* header = reinterpret_cast<PacketHeader*>(sendBuffer->Buffer());
		header->size = packetSize;
		header->id = pktId;

		bool bSuccess = pkt.SerializeToArray(sendBuffer->Buffer() + sizeof(PacketHeader), dataSize);
		if (!bSuccess) return nullptr;

		sendBuffer->Close(packetSize);

		return sendBuffer;
	}
};