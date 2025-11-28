#include "pch.h"
#include "Config.h"

#include <iostream>
#include <fstream>

ConfigLoader& GConfigLoader = ConfigLoader::Instance();

bool ConfigLoader::Load(const std::string& path)
{
    std::ifstream f(path, std::ios_base::in);
    if (!f.is_open())
    {
        std::cerr << "Failed to open config file: " << path << "\n";
        return false;
    }
    
    json j;
    f >> j;

    std::string ip = j["server"].value("ip", "127.0.0.1");
    _serverConfig.ip.assign(ip.begin(), ip.end());
    _serverConfig.port = j["server"].value("port", 7777);
    _serverConfig.maxSession = j["server"].value("max_session", 7777);
}
