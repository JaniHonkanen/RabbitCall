#include "pch.h"


void LineAndColumnNumberMap::build(const char *data, int64_t length) {
	lines.clear();
	lineBreakCounts = LineBreakCounts();
	
	int64_t pos = 0;
	int64_t currentLineNumber = 1;
	lines.emplace_back(0, 1);

	while (pos < length) {
		char ch = data[pos];
		if (ch == '\n') {
			if (pos > 0 && data[pos - 1] == '\r') {
				lineBreakCounts.crLfCount++;
			}
			else {
				lineBreakCounts.lfCount++;
			}

			currentLineNumber++;
			if (currentLineNumber > INT_MAX) throw logic_error("More than 2 billion lines in a file not supported");
			lines.emplace_back(pos + 1, (int)currentLineNumber);
		}

		pos++;
	}
}

LineAndColumnNumber LineAndColumnNumberMap::getLineAndColumnNumberByOffset(int64_t offset) {
	LineAndColumnNumber result(1, 1);
	if (lines.empty()) {
		return result;
	}

	auto iter = lower_bound(lines.begin(), lines.end(), Line(offset, 0), [](const Line &e1, const Line &e2) -> bool { return e1.offset < e2.offset; });
	if (iter == lines.end()) {
		// Rewind to the last entry.
		iter--;
	}
	else if (iter->offset > offset) {
		if (iter == lines.begin()) {
			// The offset is before the first entry.
			return result;
		}

		// The offset was not exactly at the beginning of a line => the following entry was returned => rewind to the preceding entry.
		iter--;
		if (iter->offset > offset) {
			// Should not happen, because the entry before lower_bound() should always have a key less than the search key.
			return result;
		}
	}

	result.line = iter->lineNumber;

	// Calculate column number by subtracting the line start offset from the given offset.
	// Tabs are also counted as one character in the column number, because e.g. Visual Studio will then put the cursor at the correct column when clicking an error message in the output window
	// (even though the editor will display a column number that counts tabs as several characters).
	result.column = (int)(offset - iter->offset) + 1;

	return result;
}

LineBreakCounts LineAndColumnNumberMap::getLineBreakCounts() {
	return lineBreakCounts;
}

int64_t LineAndColumnNumberMap::getLineStartOffsetByLineNumber(int64_t lineNumber) {
	if (lines.empty()) return 0;
	return lines.at(clamp(lineNumber - 1, (int64_t)0, (int64_t)lines.size() - 1)).offset;
}


