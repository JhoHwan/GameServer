#pragma once

class RecvBuffer
{
	enum { BUFFER_COUNT = 4 };
public:
	RecvBuffer(uint32 bufferSize);
	~RecvBuffer() {}

	bool OnWirte(uint32 size);
	bool OnRead(uint32 size);

	inline BYTE* WritePos() { return &_buffer[_writePos]; }
	inline BYTE* ReadPos() { return &_buffer[_readPos]; }

	inline uint32 DataSize() const { return _writePos - _readPos; }
	inline uint32 Capacity() const { return _bufferSize - _writePos; }
		
	void Clean();

private:
	//uint32
	vector<BYTE> _buffer;
	uint32 _bufferSize;

	const uint32 _maxDataSize;

	uint32 _writePos;
	uint32 _readPos;

};

