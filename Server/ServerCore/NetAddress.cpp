#include "pch.h"
#include "NetAddress.h"

NetAddress::NetAddress(SOCKADDR_IN address) : _address(address)
{
}

NetAddress::NetAddress(wstring ip, uint16 port)
{
	_address.sin_family = AF_INET;
	_address.sin_addr = Ip2Address(ip.c_str());
	_address.sin_port = ::htons(port);
}

wstring NetAddress::GetIpAddress() const
{
	WCHAR buffer[100];
	::InetNtopW(AF_INET, &_address.sin_addr, buffer, sizeof(buffer) / sizeof(WCHAR));
	return wstring(buffer);
}

IN_ADDR NetAddress::Ip2Address(const WCHAR* ip)
{
	IN_ADDR address;
	::InetPtonW(AF_INET, ip, &address);
	return address;
}
