#pragma once

class CppFile {
public:
	shared_ptr<string> path;
	int64_t fileSize = 0;
	int64_t lastModified = 0;
	bool isHeaderFile = false;

	shared_ptr<LineAndColumnNumberMap> lineAndColumnNumberMap;

	CppFile(const string &path, int64_t fileSize, int64_t lastModified, bool isHeaderFile)
		: path(make_shared<string>(path)), fileSize(fileSize), lastModified(lastModified), isHeaderFile(isHeaderFile) {
	}

	DISABLE_COPY_AND_MOVE(CppFile);
	~CppFile() = default;

	[[nodiscard]] string getPath() const { return *path; }
	SourceLocation getSourceLocationByFileOffset(int64_t offset);
	void throwParseExceptionByFileOffset(int64_t offset, const string &msg);
};


