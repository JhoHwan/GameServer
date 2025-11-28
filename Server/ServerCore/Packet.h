#pragma once

#pragma pack(1)
struct PacketHeader
{
	uint16 size;
	uint16 id;

	PacketHeader(uint16 id, uint32 size)
		: id(id), size(size) {}
};
#pragma pack()

#ifdef MY_PACKET
class IPacket 
{
public:
    virtual uint16 GetDataSize() const = 0;
    virtual bool Serialize(BYTE* buffer) const = 0;
    virtual bool Deserialize(BYTE* buffer) = 0;
    virtual ~IPacket() = default;
};

class PacketWriter
{
public:
	PacketWriter(BYTE* buffer) : _buffer(buffer), _offset(0)
	{}

	void Write(const void* buffer, uint16 len)
	{
		::memcpy(&_buffer[_offset], buffer, len);
		_offset += len;
	}

	void Write(const wstring& s)
	{
		uint16 len = static_cast<uint16>(s.size() * sizeof(wchar));
		*this << len;
		Write(s.c_str(), s.size() * sizeof(wchar));
	}

	template <typename T>
	void Write(const vector<T>& list);

	template <typename T>
	PacketWriter& operator<<(const T& data);

	template <typename T>
	PacketWriter& operator<<(const vector<T>& data);

	PacketWriter& operator<<(const wstring& data)
	{
		Write(data);
		return *this;
	}

	uint16 GetSize() const { return _offset; }
	BYTE* GetBuffer() const { return _buffer; }

private:
	BYTE* _buffer;
	uint16 _offset;
};

template <typename T>
void PacketWriter::Write(const vector<T>& list)
{
	*this << static_cast<uint16>(list.size() * sizeof(T));
	for (const auto& item : list)
	{
		*this << item;
	}
}

template <typename T>
PacketWriter& PacketWriter::operator<<(const T& data)
{
	*(reinterpret_cast<T*>(&_buffer[_offset])) = data;
	_offset += sizeof(data);
	return *this;
}

template <typename T>
PacketWriter& PacketWriter::operator<<(const vector<T>& data)
{
	Write(data);
	return *this;
}

class PacketReader
{
public:
	PacketReader(BYTE* buffer) 
		: _buffer(buffer), _offset(0)
	{}

	template<typename T>
	PacketReader& operator>>(T& data);

	template<typename T>
	PacketReader& operator>>(vector<T>& data);

	PacketReader& operator>>(wstring& data) 
	{
		uint16 len = 0;
		*this >> len;

		wchar* buffer = reinterpret_cast<wchar*>(&_buffer[_offset]);
		data = std::move(wstring(buffer, len / sizeof(wchar)));
		_offset += len;

		return *this;
	}

private:
	BYTE* _buffer;
	uint16 _offset;
};

template<typename T>
PacketReader& PacketReader::operator>>(T& data)
{
	data = *(reinterpret_cast<T*>(&_buffer[_offset]));
	_offset += sizeof(T);
	return *this;
}

template<typename T>
PacketReader& PacketReader::operator>>(vector<T>& data)
{
	uint16 len = 0;
	*this >> len;
	len /= sizeof(T);

	for (int i = 0; i < len; i++)
	{
		T d;
		*this >> d;
		data.emplace_back(d);
	}

	return *this;
}
#endif