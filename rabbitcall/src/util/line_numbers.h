#pragma once

struct LineAndColumnNumber {
	int line = 0;
	int column = 0;

	LineAndColumnNumber() = default;
	LineAndColumnNumber(int line, int column) : line(line), column(column) {
	}
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Number of line-breaks of each type.
struct LineBreakCounts {
	int64_t lfCount = 0;
	int64_t crLfCount = 0;

	void add(const LineBreakCounts &o) {
		lfCount += o.lfCount;
		crLfCount += o.crLfCount;
	}
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Maps file offsets to line/column numbers.
class LineAndColumnNumberMap {

	struct Line {
		int64_t offset;
		int lineNumber;

		Line() = default;
		Line(int64_t offset, int lineNumber) : offset(offset), lineNumber(lineNumber) {
		}
	};

	vector<Line> lines;
	LineBreakCounts lineBreakCounts;

public:
	void build(const char *data, int64_t length);
	LineAndColumnNumber getLineAndColumnNumberByOffset(int64_t offset);
	LineBreakCounts getLineBreakCounts();
	int64_t getLineStartOffsetByLineNumber(int64_t lineNumber);
};


