#pragma once

// Wrap filesystem::path because it has an implicit constructor that could accidentally convert a UTF-8 string to a path as ISO-LATIN-1.
class Path {
	filesystem::path _path;

public:
	Path() = default;
	Path(const filesystem::path &path) : _path(path) {}
	explicit Path(const string &pathUtf8) { _path = filesystem::u8path(pathUtf8); }
	explicit Path(const char *pathUtf8) { _path = filesystem::u8path(string(pathUtf8)); }
	filesystem::path path() const { return _path; }
	string toString() const { return _path.string(); }
	bool empty() const { return _path.empty(); }
};

inline Path operator/(const Path &p1, const Path &p2) {
	return p1.path() / p2.path();
}

inline StringBuilder & operator<<(StringBuilder &b, const Path &o) {
	return b << o.toString();
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Path combinePathIfRelative(Path p1, Path p2);
Path canonicalizePath(Path path);
// Much faster than Boost filesystem::relative().
string getRelativePathBetweenAbsolutePaths(const string &path, const string &base);
void convertLineBreaks(ByteBuffer *target, const void *data, int64_t length, bool useCrLfInsteadOfLfOnly);
void convertUtf8OrIsoLatinToUtf8(ByteBuffer *result, const void *data, size_t size);
string convertUtf16ToUtf8(const u16string &s);
u16string convertUtf8ToUtf16(const string &s);
void convertUtf16ToUtf8(ByteBuffer *result, const u16string &s);
void convertAutoDetectedEncodingToUtf8(ByteBuffer *result, const uint8_t *data, size_t size);

void loadBinaryFile(ByteBuffer *result, const Path &path);
void loadTextFileAsUtf8(ByteBuffer *result, const Path &path);
void saveBinaryFile(const Path &path, const void *data, size_t size);
bool isPathSameOrDescendantOf(const string &p1, const string &p2);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Closes a file automatically when this object is destroyed.
class AutoClosingFile {
	FILE *file = nullptr;

public:
	AutoClosingFile() = default;
	DISABLE_COPY(AutoClosingFile);
	AutoClosingFile(FILE *file) { *this = file; }
	AutoClosingFile(AutoClosingFile &&o) noexcept { *this = std::move(o); }
	~AutoClosingFile() { release(); }

	AutoClosingFile & operator=(FILE *file) {
		release();
		this->file = file;
		return *this;
	}

	AutoClosingFile & operator=(AutoClosingFile &&o) noexcept {
		release();
		this->file = o.file;
		o.file = nullptr;
		return *this;
	}

	void release() {
		if (file) {
			fclose(file);
			file = nullptr;
		}
	}

	operator FILE *() const {
		return file;
	}
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Parses and iterates directories in a path string, separated by a forward slash or backslash.
class PathIterator {
	string path;
	int64_t pos = 0;

	bool isDelimiter(char ch);
	
public:
	explicit PathIterator(const string &path);

	bool isFinished();
	int64_t getPosition();
	string getNextElement();
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// A set of file paths with a chain of include/exclude directives, for example:
// include /transport
// exclude /transport/vehicle
// include /transport/vehicle/car
// Contains all transports, except no other vehicles than car.
class FileSet {

	struct Element {
		string path;
		bool isInclude = false;

		Element(const string &path, bool isInclude) : path(path), isInclude(isInclude) {
		}
	};

	vector<Element> elements;

	void addElement(bool isInclude, const string &path);
	bool isAllIncludedInitially();
	
public:
	void addInclude(const string &path);
	void addExclude(const string &path);
	void initFromXml(tinyxml2::XMLElement *element);

	bool isPathInSet(const string &path);
	bool canDirectoryContainPathsInSet(const string &path);
};


