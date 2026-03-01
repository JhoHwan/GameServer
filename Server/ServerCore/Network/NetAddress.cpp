#include "NetAddress.h"

/*--------------
	NetAddress
---------------*/

NetAddress::NetAddress(SOCKADDR_IN sockAddr) : _sockAddr(sockAddr)
{
}

NetAddress::NetAddress(const string& ip, uint16 port)
{
	::memset(&_sockAddr, 0, sizeof(_sockAddr));
	_sockAddr.sin_family = AF_INET;
	_sockAddr.sin_addr = Ip2Address(ip.c_str());
	_sockAddr.sin_port = ::htons(port);
}

string NetAddress::GetIpAddress() const
{
	char buffer[100];
	::inet_ntop(AF_INET, &_sockAddr.sin_addr, buffer, len32(buffer));

	return string(buffer);
}

IN_ADDR NetAddress::Ip2Address(const char* ip)
{
	IN_ADDR address;
	::inet_pton(AF_INET, ip, &address);
	return address;
}
