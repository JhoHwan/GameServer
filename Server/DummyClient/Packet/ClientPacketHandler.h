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
	PKT_CS_USE_PORTAL = 1003,
	PKT_SC_MOVE_FIELD_FAIL = 1004,
	PKT_SC_START_FIELD_LOADING = 1005,
	PKT_CS_FIELD_LOADING_COMPLETE = 1006,
	PKT_SC_ENTER_FIELD = 1007,
	PKT_SC_SPAWN_PLAYER = 1008,
	PKT_SC_DESPAWN_PLAYER = 1009,
	PKT_CS_REQUEST_MOVE = 1010,
	PKT_SC_MOVE_PATH = 1011,
	PKT_CS_TIME_SYNC = 1012,
	PKT_SC_TIME_SYNC = 1013,
};

// Custom Handlers
bool Handle_INVALID(SessionRef& session, BYTE* buffer, int32 len);
bool Handle_SC_ENTER_GAME_RESULT(SessionRef& session, Protocol::SC_ENTER_GAME_RESULT& pkt);
bool Handle_SC_MOVE_FIELD_FAIL(SessionRef& session, Protocol::SC_MOVE_FIELD_FAIL& pkt);
bool Handle_SC_START_FIELD_LOADING(SessionRef& session, Protocol::SC_START_FIELD_LOADING& pkt);
bool Handle_SC_ENTER_FIELD(SessionRef& session, Protocol::SC_ENTER_FIELD& pkt);
bool Handle_SC_SPAWN_PLAYER(SessionRef& session, Protocol::SC_SPAWN_PLAYER& pkt);
bool Handle_SC_DESPAWN_PLAYER(SessionRef& session, Protocol::SC_DESPAWN_PLAYER& pkt);
bool Handle_SC_MOVE_PATH(SessionRef& session, Protocol::SC_MOVE_PATH& pkt);
bool Handle_SC_TIME_SYNC(SessionRef& session, Protocol::SC_TIME_SYNC& pkt);

class ClientPacketHandler
{
public:
	static void Init()
	{
		for (int32 i = 0; i < UINT16_MAX; i++)
			GPacketHandler[i] = Handle_INVALID;
		GPacketHandler[PKT_SC_ENTER_GAME_RESULT] = [](SessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::SC_ENTER_GAME_RESULT>(Handle_SC_ENTER_GAME_RESULT, session, buffer, len); };
		GPacketHandler[PKT_SC_MOVE_FIELD_FAIL] = [](SessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::SC_MOVE_FIELD_FAIL>(Handle_SC_MOVE_FIELD_FAIL, session, buffer, len); };
		GPacketHandler[PKT_SC_START_FIELD_LOADING] = [](SessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::SC_START_FIELD_LOADING>(Handle_SC_START_FIELD_LOADING, session, buffer, len); };
		GPacketHandler[PKT_SC_ENTER_FIELD] = [](SessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::SC_ENTER_FIELD>(Handle_SC_ENTER_FIELD, session, buffer, len); };
		GPacketHandler[PKT_SC_SPAWN_PLAYER] = [](SessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::SC_SPAWN_PLAYER>(Handle_SC_SPAWN_PLAYER, session, buffer, len); };
		GPacketHandler[PKT_SC_DESPAWN_PLAYER] = [](SessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::SC_DESPAWN_PLAYER>(Handle_SC_DESPAWN_PLAYER, session, buffer, len); };
		GPacketHandler[PKT_SC_MOVE_PATH] = [](SessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::SC_MOVE_PATH>(Handle_SC_MOVE_PATH, session, buffer, len); };
		GPacketHandler[PKT_SC_TIME_SYNC] = [](SessionRef& session, BYTE* buffer, int32 len) { return HandlePacket<Protocol::SC_TIME_SYNC>(Handle_SC_TIME_SYNC, session, buffer, len); };
	}

	static bool HandlePacket(SessionRef& session, BYTE* buffer, int32 len)
	{
		PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);
		return GPacketHandler[header->id](session, buffer, len);
	}
	static SendBufferRef MakeSendBuffer(Protocol::CS_REQ_ENTER_GAME& pkt) { return MakeSendBuffer(pkt, PKT_CS_REQ_ENTER_GAME); }
	static SendBufferRef MakeSendBuffer(Protocol::CS_REQ_MOVE_FIELD& pkt) { return MakeSendBuffer(pkt, PKT_CS_REQ_MOVE_FIELD); }
	static SendBufferRef MakeSendBuffer(Protocol::CS_USE_PORTAL& pkt) { return MakeSendBuffer(pkt, PKT_CS_USE_PORTAL); }
	static SendBufferRef MakeSendBuffer(Protocol::CS_FIELD_LOADING_COMPLETE& pkt) { return MakeSendBuffer(pkt, PKT_CS_FIELD_LOADING_COMPLETE); }
	static SendBufferRef MakeSendBuffer(Protocol::CS_REQUEST_MOVE& pkt) { return MakeSendBuffer(pkt, PKT_CS_REQUEST_MOVE); }
	static SendBufferRef MakeSendBuffer(Protocol::CS_TIME_SYNC& pkt) { return MakeSendBuffer(pkt, PKT_CS_TIME_SYNC); }

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