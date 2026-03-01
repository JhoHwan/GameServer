#pragma once

/*--------------
	NetAddress
---------------*/

class NetAddress
{
public:
	NetAddress() = default;
	NetAddress(SOCKADDR_IN sockAddr);
	NetAddress(const string& ip, uint16 port);

	SOCKADDR_IN& GetSockAddr() { return _sockAddr; }
	string GetIpAddress() const;
	uint16 GetPort() const { return ::ntohs(_sockAddr.sin_port); }

public:
	static IN_ADDR Ip2Address(const char* ip);

private:
	SOCKADDR_IN		_sockAddr = {};
};

