#pragma once

class ByteBuffer {
	uint8_t *buffer = nullptr;
	size_t bufferSize = 0;
	size_t pos = 0;

public:
	ByteBuffer();
	explicit ByteBuffer(size_t size);
	DISABLE_COPY_AND_MOVE(ByteBuffer);
	~ByteBuffer();

	void ensureRemainingCapacity(size_t amount);
	void resize(size_t newSize);

	size_t getBufferSize();
	size_t getBytesRemaining();
	void checkBytesRemaining(size_t length);
	void setPosition(size_t newPos);
	size_t getPosition();
	uint8_t *getBuffer();
	void compact();

	template<typename T>
	void put(const T &value) {
		ensureRemainingCapacity(sizeof(value));
		const size_t valueSize = sizeof value;
		*((T *)(buffer + pos)) = value;
		pos += valueSize;
	}

	template<typename T>
	void get(T &value) {
		const size_t valueSize = sizeof(T);
		if (pos + valueSize > bufferSize) EXC("Unexpected end-of-data");
		value = *((T *)(buffer + pos));
		pos += valueSize;
	}

	template<typename T>
	const T & get() {
		const size_t valueSize = sizeof(T);
		if (pos + valueSize > bufferSize) EXC("Unexpected end-of-data");
		const T &value = *((T *)(buffer + pos));
		pos += valueSize;
		return value;
	}

	void getBytes(void *targetBuffer, size_t length);
	const void * skipBytes(size_t length);
	void getBytes(ByteBuffer &targetBuffer, size_t length);
	void putBytes(const void *sourceBuffer, size_t length);

	uint64_t getVlq();
	void putVlq(uint64_t value);
};


