#pragma once

struct ServerConfig
{
	std::wstring Ip;
	uint16 Port;

	int32 NumOfIOCPThread;
	int32 NumOfWorkerThread;
};

class ConfigLoader : public Singleton<ConfigLoader>
{
	friend class Singleton<ConfigLoader>;

public:
	bool Load(const std::wstring& path);
	const ServerConfig& GetServerConfig() const { return _serverConfig; }

private:
	ConfigLoader() = default;
	~ConfigLoader() = default;

private:
	ServerConfig _serverConfig;
};

extern class ConfigLoader& GConfigLoader;