#pragma once

class StringBuilder {
public:
	string buffer;
	int lineIndent = 0;
	static const char indentChar = '\t';

	StringBuilder() {
	}

	StringBuilder(const StringBuilder &other) = delete;
	StringBuilder & operator=(const StringBuilder &other) = delete;

	StringBuilder(StringBuilder &&other) noexcept {
		*this = std::move(other);
	}

	StringBuilder & operator=(StringBuilder &&other) noexcept {
		buffer = move(other.buffer);
		lineIndent = other.lineIndent;
		return *this;
	}

	~StringBuilder() {
	}

	StringBuilder & changeIndent(int delta) {
		lineIndent += delta;
		return *this;
	}

	StringBuilder & appendIndent() {
		for (int i = 0; i < lineIndent; i++) {
			buffer.push_back(indentChar);
		}
		return *this;
	}

	StringBuilder & appendLine(const char *s) {
		appendIndent();
		buffer.append(s);
		buffer.push_back('\n');
		return *this;
	}

	StringBuilder & appendLine(const string &s) {
		appendIndent();
		buffer.append(s);
		buffer.push_back('\n');
		return *this;
	}

	// ReSharper disable once CppNonExplicitConversionOperator
	operator string() const {
		return buffer;
	}

	shared_ptr<ByteBuffer> getDataCopy() {
		shared_ptr<ByteBuffer> result = make_shared<ByteBuffer>();
		result->putBytes(buffer.data(), buffer.size());
		return result;
	}
};

// When a string is built using a temporary object in an expression (e.g. "sb() << "abc" << x"), the object can be used as an lvalue reference on MSVC, but not e.g. on GCC
// => define an operator<< that takes an rvalue reference (&&) instead and invokes the corresponding lvalue reference (&) operator.
template<typename T>
StringBuilder & operator<<(StringBuilder &&b, const T &o) { return operator<<(b, o); }

inline StringBuilder & operator<<(StringBuilder &b, const string &o) { b.buffer.append(o); return b; }
inline StringBuilder & operator<<(StringBuilder &b, const char *o) { b.buffer.append(o); return b; }
inline StringBuilder & operator<<(StringBuilder &b, char o) { b.buffer.push_back(o); return b; }
inline StringBuilder & operator<<(StringBuilder &b, int64_t o) { b.buffer.append(to_string(o)); return b; }
inline StringBuilder & operator<<(StringBuilder &b, uint64_t o) { b.buffer.append(to_string(o)); return b; }
inline StringBuilder & operator<<(StringBuilder &b, int32_t o) { b.buffer.append(to_string(o)); return b; }
inline StringBuilder & operator<<(StringBuilder &b, uint32_t o) { b.buffer.append(to_string(o)); return b; }
inline StringBuilder & operator<<(StringBuilder &b, double o) { b.buffer.append(to_string(o)); return b; }
inline StringBuilder & operator<<(StringBuilder &b, float o) { b.buffer.append(to_string(o)); return b; }

template<typename T>
StringBuilder & operator<<(StringBuilder &b, const vector<T> &o) {
	b << '{';
	bool first = true;
	for (const T &key : o) {
		if (!first) b << ", ";
		first = false;
		b << key;
	}
	b << '}';
	return b;
}

inline StringBuilder sb() {
	return StringBuilder();
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class StringJoiner {
	StringBuilder *buffer = nullptr;
	bool isEmpty = true;

	void checkIfFirstElement() {
		if (isEmpty) {
			isEmpty = false;
			*buffer << prefixIfNotEmpty;
		}
		else {
			*buffer << delimiter;
		}
	}

public:
	string delimiter;
	string prefixIfNotEmpty;
	string suffixIfNotEmpty;

	StringJoiner(StringBuilder *buffer, const string &delimiter) : buffer(buffer), delimiter(delimiter) {
	}

	void append(const string &s) {
		checkIfFirstElement();
		*buffer << s;
	}

	void append(const char *s) {
		checkIfFirstElement();
		*buffer << s;
	}

	void finish() {
		if (!isEmpty) {
			*buffer << suffixIfNotEmpty;
		}
	}
};

