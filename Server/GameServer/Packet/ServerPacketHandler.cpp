#include "pch.h"
#include "ServerPacketHandler.h"
#include "Contents\Player.h"
#include "Contents\Field.h"
#include "GameSession.h"

PacketHandlerFunc GPacketHandler[UINT16_MAX];

bool Handle_INVALID(SessionRef& session, BYTE* buffer, int32 len)
{
    return false;
}

bool Handle_CS_REQ_ENTER_GAME(SessionRef& session, Protocol::CS_REQ_ENTER_GAME& pkt)
{
    shared_ptr<GameSession> gSession = static_pointer_cast<GameSession>(session);
    
    gSession->CancelTimeOut();

    gSession->GetJobQueue()->DoAsync([gSession]()
        {
            {
                // TODO : 토큰 인증 & DB 요청 후 아래 내용 콜백으로 등록
                Protocol::SC_ENTER_GAME_RESULT packet;
                packet.set_success(true); // 추후 DB 요청 결과 or 인증 결과에 따라 변경
                gSession->Send(ServerPacketHandler::MakeSendBuffer(packet));
            }
            {
                auto player = GameObject::Create<PlayerCharacter>(gSession);
                gSession->SetPlayer(player);

                Protocol::SC_START_FIELD_LOADING packet;
                packet.set_target_map_id(0);
                packet.mutable_start_pos()->CopyFrom(Vector3::Zero().ToProto());
                gSession->Send(ServerPacketHandler::MakeSendBuffer(packet));
                gSession->SetTimeOut(60000, L"Map Loading");
            }
        });
    return true;
}

bool Handle_CS_REQ_MOVE_FIELD(SessionRef& session, Protocol::CS_REQ_MOVE_FIELD& pkt)
{
    shared_ptr<GameSession> gSession = static_pointer_cast<GameSession>(session);
    gSession->GetJobQueue()->DoAsync([gSession]()
        {
            Protocol::SC_START_FIELD_LOADING packet;
            packet.set_target_map_id(0);
            packet.mutable_start_pos()->CopyFrom(Vector3::Zero().ToProto());
            gSession->Send(ServerPacketHandler::MakeSendBuffer(packet));
            gSession->SetTimeOut(60000, L"Map Loading");
        });
    return true;
}

bool Handle_CS_FIELD_LOADING_COMPLETE(SessionRef& session, Protocol::CS_FIELD_LOADING_COMPLETE& pkt)
{
    shared_ptr<GameSession> gSession = static_pointer_cast<GameSession>(session);
    gSession->CancelTimeOut();
    gSession->GetJobQueue()->DoAsync([gSession]()
        {
            shared_ptr<PlayerCharacter> player = gSession->GetPlayer();
            if (!player) gSession->Disconnect(L"Invalid Error");

            Protocol::SC_ENTER_FIELD packet;
            player->GetObjectInfo(packet.mutable_my_info()->mutable_object_info());
            SendBufferRef sendBuffer = ServerPacketHandler::MakeSendBuffer(packet);
            auto session = player->GetSession();
            if (session) session->Send(sendBuffer);

            GFieldManager.Find(0)->EnterPlayer(player);
        });
    return true;
}

bool Handle_CS_MOVE(SessionRef& session, Protocol::CS_MOVE& pkt)
{
    return false;
}

bool Handle_CS_PING(SessionRef& session, Protocol::CS_PING& pkt)
{
    cout << "Ping! : " << pkt.id() << endl;

    Protocol::SC_PONG packet;
    packet.set_id(pkt.id());
    auto buffer = ServerPacketHandler::MakeSendBuffer(packet);
    if (!buffer) return false;
    session->Send(buffer);

    return true;
}
