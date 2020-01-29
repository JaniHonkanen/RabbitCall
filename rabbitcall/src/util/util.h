#pragma once

#define DISABLE_COPY(T) \
	T(const T &) = delete;\
	T & operator=(const T &) = delete
#define DISABLE_COPY_AND_MOVE(T) \
	DISABLE_COPY(T);\
	T(T &&) = delete;\
	T & operator=(T &&) = delete

class Guard {
	function<void()> f;
public:
	Guard(const function<void()> &f) : f(f) {}
	DISABLE_COPY_AND_MOVE(Guard);
	~Guard() { f(); }
};

#define EXC(msg) throw runtime_error(string(msg).c_str())

#define CURRENT_SOURCE_LOCATION (string(__FILE__) + ":" + to_string(__LINE__) + " --- " + string(__func__))
#define CHECK_NOT_NULL(p) if (!p) EXC(sb() << "Null pointer: " << #p << " (" << typeid(p).name() << ", " << CURRENT_SOURCE_LOCATION << ")")

struct PointerAndSize {
	void *ptr;
	int64_t size;

	PointerAndSize(void *ptr, int64_t size) : ptr(ptr), size(size) {
	}
};

void * allocateMemory(size_t size);
void * reallocateMemory(void *oldPtr, size_t size);
void freeMemory(void *ptr);

template<typename T, typename C>
bool contains(const C &set, const T &element) {
	return set.find(element) != set.end();
}

template<class C1, class C2>
void addAll(C1 &targetCollection, const C2 &sourceCollection) {
	for (auto o : sourceCollection) targetCollection.push_back(o);
}

template<typename Key, typename Value, typename Comparator, typename Alloc>
Value getIfExists(const Key &key, const map<Key, Value, Comparator, Alloc> &_map) {
	typename map<Key, Value, Comparator, Alloc>::const_iterator i = _map.find(key);
	if (i == _map.end()) return Value();
	return i->second;
}

double getTimeSeconds();

bool isPowerOfTwo(int64_t v);
int64_t alignOffsetToNextBoundary(int64_t offset, int64_t alignment);

struct StringView {
	const char *ptr;
	int64_t length;

	StringView(const char *ptr, int64_t length) : ptr(ptr), length(length) {
	}

	bool operator==(const char *s) const;
	string toString() const;
};

bool parseBool(const string &s);
bool tryParseInt64(const string &s, int64_t *result);

string nullToEmpty(const char *s);



