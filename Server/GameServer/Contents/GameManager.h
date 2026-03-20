#pragma once

class GameManager : public Singleton<GameManager>, public AsyncActor
{
public:
    GameManager() = default;
    ~GameManager() override = default;

    void ProcessEnterGame(const std::weak_ptr<class GameSession>& sessionRef);
    void ProcessMoveField(const std::shared_ptr<class PlayerCharacter>& player, uint16 targetMapId, int32 targetPortalId = -1);
};
