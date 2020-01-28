#include "pch.h"


ByteBuffer::ByteBuffer()
	: ByteBuffer(0) {
}

ByteBuffer::ByteBuffer(size_t size) {
	bufferSize = size;
	if (size == 0) {
		buffer = nullptr;
	}
	else {
		buffer = (uint8_t *)allocateMemory(bufferSize);
	}
	pos = 0;
}

ByteBuffer::~ByteBuffer() {
	if (buffer) {
		free(buffer);
		buffer = nullptr;
	}
}

void ByteBuffer::setPosition(size_t newPos) {
	if (newPos < 0 || newPos > bufferSize) EXC(sb() << "ByteBuffer::setPosition() - newPos must be within 0.." << bufferSize);
	pos = newPos;
}

size_t ByteBuffer::getPosition() {
	return pos;
}

uint8_t * ByteBuffer::getBuffer() {
	return buffer;
}

void ByteBuffer::ensureRemainingCapacity(size_t amount) {
	if (pos + amount > bufferSize) {
		size_t newSize = bufferSize;
		while (pos + amount > newSize) {
			newSize = max(newSize * 2, (size_t)64);
		}
		resize(newSize);
	}
}

void ByteBuffer::resize(size_t newSize) {
	if (newSize != bufferSize) {
		if (newSize == 0) {
			free(buffer);
			buffer = nullptr;
		}
		else {
			buffer = (uint8_t *)reallocateMemory(buffer, newSize);
		}
		bufferSize = newSize;
		pos = min(pos, bufferSize);
	}
}

size_t ByteBuffer::getBufferSize() {
	return bufferSize;
}

size_t ByteBuffer::getBytesRemaining() {
	return bufferSize - pos;
}

void ByteBuffer::checkBytesRemaining(size_t length) {
	size_t remaining = getBytesRemaining();
	if (remaining < length) EXC("Unexpected end-of-data");
}

void ByteBuffer::compact() {
	resize(pos);
}

void ByteBuffer::getBytes(void *targetBuffer, size_t length) {
	checkBytesRemaining(length);
	memcpy(targetBuffer, &buffer[pos], length);
	pos += length;
}

const void * ByteBuffer::skipBytes(size_t length) {
	checkBytesRemaining(length);
	void *ptr = &buffer[pos];
	pos += length;
	return ptr;
}

void ByteBuffer::getBytes(ByteBuffer &targetBuffer, size_t length) {
	targetBuffer.ensureRemainingCapacity(length);
	getBytes(&targetBuffer.buffer[targetBuffer.pos], length);
	targetBuffer.pos += length;
}

void ByteBuffer::putBytes(const void *sourceBuffer, size_t length) {
	ensureRemainingCapacity(length);
	memcpy(&buffer[pos], sourceBuffer, length);
	pos += length;
}

uint64_t ByteBuffer::getVlq() {
	uint64_t result = 0;
	while (true) {
		uint64_t t = get<uint8_t>();
		if ((result << 7) >> 7 != result) EXC("VLQ number too large.");
		result = (result << 7) | (t & 0x7F);

		if ((t & 0x80) == 0) break;
	}
	return result;
}

void ByteBuffer::putVlq(uint64_t value) {
	int i = 9 * 7;
	for (; i >= 0; i -= 7) {
		if (((value >> i) & 0x7F) != 0) break;
	}
	if (i < 0) {
		put((uint8_t)0);
	}
	else {
		for (; i >= 0; i -= 7) {
			uint8_t p = (uint8_t)((value >> i) & 0x7F);
			if (i > 0) p |= 0x80;
			put((uint8_t)p);
		}
	}
}



