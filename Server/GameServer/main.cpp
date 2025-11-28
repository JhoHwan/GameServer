#include "pch.h"
#include <nlohmann/json.hpp>

#include <iostream>
#include <fstream>
#include <memory>
#include <map>
#include <string>
#include "Service.h"
#include "GameSession.h"
#include "Util/Config/Config.h"
#include "Util/Log/Logger.h"


int main()
{
	if (!GConfigLoader.Load("Config.json"))
	{
		return -1;
	}
	const auto& serverConfig = GConfigLoader.GetServerConfig();

	GLogger.Init();

	LOG_GAME_ERROR("Test");

	GLogger.Shutdown();
}