#include "pch.h"


SourceLocation CppFile::getSourceLocationByFileOffset(int64_t offset) {
	return SourceLocation(path, lineAndColumnNumberMap->getLineAndColumnNumberByOffset(offset));
}

void CppFile::throwParseExceptionByFileOffset(int64_t offset, const string &msg) {
	throw ParseException(getSourceLocationByFileOffset(offset), msg);
}

