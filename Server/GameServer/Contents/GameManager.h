#pragma once

class GameManager : public Singleton<GameManager>, public AsyncActor
{
public:
    ~GameManager() = default;

    void ProcessEnterGame(std::weak_ptr<class GameSession> session);
    void ProcessMoveField(const std::shared_ptr<class PlayerCharacter>& player, uint16 targetMapId, int32 targetPortalId = -1);
};
