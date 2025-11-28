#pragma once
class NetAddress
{
public:
    // 생성자
    NetAddress() {};
    NetAddress(SOCKADDR_IN address);
    NetAddress(wstring ip, uint16 port);

    // Getter
    SOCKADDR_IN GetAddress() const { return _address; }
    std::wstring GetIpAddress() const;
    uint16_t GetPort() const { return ::ntohs(_address.sin_port); }

    // 유틸리티
    static IN_ADDR Ip2Address(const WCHAR* ip);

private:
    SOCKADDR_IN _address = {};
};

