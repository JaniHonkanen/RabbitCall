#include "pch.h"


Path combinePathIfRelative(Path p1, Path p2) {
	return p2.path().is_absolute() ? p2 : (p1 / p2);
}

Path canonicalizePath(Path path) {
	return weakly_canonical(path.path()).make_preferred();
}

string getRelativePathBetweenAbsolutePaths(const string &path, const string &base) {
	PathIterator p1(path);
	PathIterator p2(base);
	int64_t pos1 = 0;
	int64_t numParentTransitions = 0;
	while (true) {
		pos1 = p1.getPosition();
		
		if (p1.isFinished() || p2.isFinished()) {
			break;
		}

		string name1 = p1.getNextElement();
		string name2 = p2.getNextElement();
		if (name1 != name2) {
			numParentTransitions = 1;
			break;
		}
	}

	if (pos1 == 0) {
		// If there were no same path elements even in the beginning, return the original path,
		// because it may be on a different drive or in a different user's directory.
		return path;
	}

	while (!p2.isFinished()) {
		numParentTransitions++;
		p2.getNextElement();
	}

	char delimiter = '/';
	
	string result;
	for (int64_t i = 0; i < numParentTransitions; i++) {
		if (i > 0) result += delimiter;
		result += "..";
	}

	if (!result.empty() && pos1 < (int64_t)path.size()) result += delimiter;
	result += path.substr(pos1);

	return Path(result).path().make_preferred().string();
}

void convertLineBreaks(ByteBuffer *target, const void *data, int64_t length, bool useCrLfInsteadOfLfOnly) {
	const uint8_t *sourceData = (const uint8_t *)data;
	int64_t lineStart = 0;

	auto copyCurrentLine = [&](int64_t lineEnd, int64_t nextLineStart) {
		if (lineEnd > lineStart) {
			target->putBytes(sourceData + lineStart, lineEnd - lineStart);
		}
		lineStart = nextLineStart;
	};

	for (int64_t i = 0; i < length; i++) {
		char ch = sourceData[i];
		if (ch == '\n') {
			copyCurrentLine(i, i + 1);
			if (useCrLfInsteadOfLfOnly) target->put<char>('\r');
			target->put<char>('\n');
		}
		else if (ch == '\r') {
			copyCurrentLine(i, i + 1);
		}
	}
	copyCurrentLine(length, length);
}

void convertUtf8OrIsoLatinToUtf8(ByteBuffer *result, const void *data, size_t size) {
	const char *ptr = (const char *)data;
	int64_t length = (int64_t)size;
	for (int64_t i = 0; i < length;) {
		unsigned char ch = ptr[i++];

		if ((ch & 0x80) == 0) {
			// ASCII 7-bit char.
			result->put(ch);
			continue;
		}

		// Check if this is a valid multi-byte UTF-8 char.
		if ((ch & 0xE0) == 0xC0) {
			if (i < length) {
				char ch2 = ptr[i];
				if ((ch2 & 0xC0) == 0x80) {
					result->put(ch);
					result->put(ch2);
					i += 1;
					continue;
				}
			}
		}
		else if ((ch & 0xF0) == 0xE0) {
			if (i + 1 < length) {
				char ch2 = ptr[i];
				char ch3 = ptr[i + 1];
				if ((ch2 & 0xC0) == 0x80 && (ch3 & 0xC0) == 0x80) {
					result->put(ch);
					result->put(ch2);
					result->put(ch3);
					i += 2;
					continue;
				}
			}
		}
		else if ((ch & 0xF8) == 0xF0) {
			if (i + 2 < length) {
				char ch2 = ptr[i];
				char ch3 = ptr[i + 1];
				char ch4 = ptr[i + 2];
				if ((ch2 & 0xC0) == 0x80 && (ch3 & 0xC0) == 0x80 && (ch4 & 0xC0) == 0x80) {
					result->put(ch);
					result->put(ch2);
					result->put(ch3);
					result->put(ch4);
					i += 3;
					continue;
				}
			}
		}

		// Invalid UTF-8 char => assume it is ISO-Latin-1 and convert to UTF-8.
		result->put<uint8_t>(0xc0 | (ch >> 6));
		result->put<uint8_t>(0x80 | (ch & 0x3F));
	}
}

string convertUtf16ToUtf8(const u16string &s) {
	string result;
	utf8::utf16to8((const char16_t *)s.data(), (const char16_t *)s.data() + s.size(), back_inserter(result));
	return result;
}

u16string convertUtf8ToUtf16(const string &s) {
	u16string result;
	utf8::utf8to16((const char *)s.data(), (const char *)s.data() + s.size(), back_inserter(result));
	return result;
}

void convertUtf16ToUtf8(ByteBuffer *result, const u16string &s) {
	string utf8 = convertUtf16ToUtf8(s);
	result->putBytes(utf8.data(), utf8.size());
}

void convertAutoDetectedEncodingToUtf8(ByteBuffer *result, const uint8_t *data, size_t size) {
	// Try to determine encoding by BOM, and leave the BOM out of the result.
	if (size >= 3 && data[0] == 0xEF && data[1] == 0xBB && data[2] == 0xBF) { // UTF-8
		size_t bomSize = 3;
		result->putBytes(data + bomSize, size - bomSize);
	}
	else if (size >= 2 && *(char16_t *)data == 0xFEFF) { // UTF-16 LE (or BE on big-endian machine)
		size_t bomSizeChars = 1;
		convertUtf16ToUtf8(result, u16string((const char16_t *)data + bomSizeChars, size / 2 - bomSizeChars));
	}
	else if (size >= 2 && *(char16_t *)data == 0xFFFE) { // UTF-16 BE (or LE on big-endian machine)
		ByteBuffer temp;
		temp.resize(size);
		uint8_t *dst = temp.getBuffer();
		for (size_t i = 0; i < size; i += 2) {
			char16_t t = *(char16_t *)(data + i);
			t = ((t & 0xFF) << 8) | (t >> 8);
			*(char16_t *)(dst + i) = t;
		}

		size_t bomSizeChars = 1;
		convertUtf16ToUtf8(result, u16string((const char16_t *)temp.getBuffer() + bomSizeChars, size / 2 - bomSizeChars));
	}
	else {
		convertUtf8OrIsoLatinToUtf8(result, data, size);
	}
}

void loadBinaryFile(ByteBuffer *result, const Path &path) {
	int64_t size;
	AutoClosingFile file = openFileOrThrow(path.path(), "rb", &size);
	result->ensureRemainingCapacity(size);
	int64_t bytesRead = fread(result->getBuffer(), 1, size, file);
	if (bytesRead < size) EXC(sb() << "Error reading file: " << path);
	result->setPosition(size);
}

void loadTextFileAsUtf8(ByteBuffer *result, const Path &path) {
	StopWatch stopWatch = app->createStopWatchForPerformanceMeasurement();
	ByteBuffer data;
	loadBinaryFile(&data, path);
	stopWatch.mark("load text file / disk");
	convertAutoDetectedEncodingToUtf8(result, data.getBuffer(), data.getPosition());
	stopWatch.mark("load text file / convert to utf-8");
}

void saveBinaryFile(const Path &path, const void *data, size_t size) {
	Path parentPath = Path(path).path().parent_path();
	if (!parentPath.empty() && !exists(parentPath.path())) {
		EXC(sb() << "Directory does not exist for writing file: " << path);
	}

	AutoClosingFile file = openFileOrThrow(path.path(), "wb", nullptr);
	size_t bytesWritten = fwrite(data, 1, size, file);
	if (bytesWritten < size) EXC(sb() << "Error writing file: " << path);
}

bool isPathSameOrDescendantOf(const string &p1, const string &p2) {
	PathIterator i1(p1);
	PathIterator i2(p2);
	while (true) {
		if (i1.isFinished()) {
			return i2.isFinished();
		}
		else if (i2.isFinished()) {
			return true;
		}

		if (i1.getNextElement() != i2.getNextElement()) {
			return false;
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool PathIterator::isDelimiter(char ch) {
	return ch == '/' || ch == '\\';
}

PathIterator::PathIterator(const string &path)
	: path(path) {
}

bool PathIterator::isFinished() {
	return pos >= (int64_t)path.size();
}

int64_t PathIterator::getPosition() {
	return pos;
}

string PathIterator::getNextElement() {
	if (isFinished()) EXC(sb() << "Attempted to iterate path after finished: " << path);

	int64_t startPos = pos;
	int64_t endPos = pos;
	for (int64_t p = pos;; p++) {
		if (p >= (int64_t)path.size()) {
			endPos = p;
			pos = p;
			break;
		}

		if (isDelimiter(path[p])) {
			endPos = p;
			pos = p + 1;
			break;
		}
	}

	return path.substr(startPos, endPos - startPos);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void FileSet::addElement(bool isInclude, const string &path) {
	elements.emplace_back(path, isInclude);
}

bool FileSet::isAllIncludedInitially() {
	// If the first element is an include, consider everything excluded until explicitly included,
	// otherwise consider everything included until explicitly exlcluded.
	if (elements.empty()) return true;
	return !elements.at(0).isInclude;
}

void FileSet::addInclude(const string &path) {
	addElement(true, path);
}

void FileSet::addExclude(const string &path) {
	addElement(false, path);
}

void FileSet::initFromXml(tinyxml2::XMLElement *element) {
	element->forEachChildElement([&](tinyxml2::XMLElement *xmlItem) {
		const char *namePtr = xmlItem->Name();
		string name = string(namePtr ? namePtr : "");
		string path = xmlItem->GetAttributeIfExists("path");

		if (name == "include") {
			addInclude(path);
		}
		else if (name == "exclude") {
			addExclude(path);
		}
	});
}

bool FileSet::isPathInSet(const string &path) {
	bool isIncluded = isAllIncludedInitially();
	for (Element &element : elements) {
		if (isPathSameOrDescendantOf(path, element.path)) {
			isIncluded = element.isInclude;
		}
	}
	return isIncluded;
}

bool FileSet::canDirectoryContainPathsInSet(const string &path) {
	bool isIncluded = isAllIncludedInitially();
	for (Element &element : elements) {
		if (element.isInclude) {
			if (isPathSameOrDescendantOf(path, element.path) || isPathSameOrDescendantOf(element.path, path)) {
				isIncluded = true;
			}
		}
		else {
			if (isPathSameOrDescendantOf(path, element.path)) {
				isIncluded = false;
			}
		}
	}
	return isIncluded;
}

