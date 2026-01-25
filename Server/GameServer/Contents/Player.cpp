#include "pch.h"
#include "Player.h"
#include "GameSession.h"

void PlayerCharacter::Init()
{
	GameObject::Init();

	auto session = GetSession();
	auto player = static_pointer_cast<PlayerCharacter>(shared_from_this());
	if (session) session->SetPlayer(player);
}