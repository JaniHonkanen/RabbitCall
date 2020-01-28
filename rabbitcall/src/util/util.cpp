#include "pch.h"

void * allocateMemory(size_t size) {
	void *ptr = malloc(size);
	if (ptr == nullptr) throw bad_alloc();
	return ptr;
}

void * reallocateMemory(void *oldPtr, size_t size) {
	if (!oldPtr) {
		return allocateMemory(size);
	}
	void *newPtr = realloc(oldPtr, size);
	if (newPtr == nullptr) throw bad_alloc();
	return newPtr;
}

void freeMemory(void *ptr) {
	if (ptr) {
		free(ptr);
	}
}

double getTimeSeconds() {
	return chrono::duration<double>(chrono::high_resolution_clock::now().time_since_epoch()).count();
}

bool isPowerOfTwo(int64_t v) {
	return (v > 0) && ((v & (v - 1)) == 0);
}

int64_t alignOffsetToNextBoundary(int64_t offset, int64_t alignment) {
	int64_t excess = offset % alignment;
	if (excess > 0) {
		offset += alignment - excess;
	}
	return offset;
}

bool StringView::operator==(const char *s) const {
	for (int64_t i = 0; i < length; i++) {
		char ch = *s;
		if (ch == '\0' || ch != ptr[i]) return false;
		s++;
	}
	if (*s != '\0') return false;
	return true;
}

string StringView::toString() const {
	return string(ptr, length);
}

bool parseBool(const string &s) {
	return s == "true";
}

bool tryParseInt64(const string &s, int64_t *result) {
	int64_t r = 0;
	bool negative = false;
	size_t length = s.size();

	size_t i = 0;
	for (; i < length && s[i] == ' '; i++);
	if (i < length && s[i] == '-') {
		negative = true;
		i++;
	}
	else if (i < length && s[i] == '+') {
		i++;
	}
	for (; i < length && s[i] == ' '; i++);

	bool digitsFound = false;
	for (; i < length; i++) {
		char ch = s[i];
		if (ch < '0' || ch > '9') break;

		int64_t next = r * 10;
		if (next / 10 != r) return false; // Overflow
		r = next + (ch - '0');
		digitsFound = true;
	}

	for (; i < length && s[i] == ' '; i++);

	if (!digitsFound || i < length) return false;
	if (result) *result = negative ? -r : r;
	return true;
}

string nullToEmpty(const char *s) {
	return s ? s : string();
}



