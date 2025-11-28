#pragma once

#include "../Singleton.h"

struct ServerConfig
{
	std::wstring ip;
	uint16 port;
	uint32 maxSession;
};

class ConfigLoader : public Singleton<ConfigLoader>
{
	friend class Singleton<ConfigLoader>;

public:
	bool Load(const std::string& path);
	const ServerConfig& GetServerConfig() const { return _serverConfig; }

private:
	ConfigLoader() = default;
	~ConfigLoader() = default;

private:
	ServerConfig _serverConfig;
};

extern class ConfigLoader& GConfigLoader;