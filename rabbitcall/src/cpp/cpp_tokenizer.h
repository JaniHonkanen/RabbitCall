#pragma once

class CppTokenizer;

const int CPP_INVALID_BLOCK_DEPTH = -1;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct CppStringLiteralType {
	// Other types than raw strings are not currently parsed specially.
	bool isValid = false;
	bool isRaw = false;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Position of an element in source code and related parsing state that is needed when moving an iterator to that position.
struct CppElementIndex {
	int64_t pos = -1;

	// How many nested parentheses/brackets/braces there are enclosing the current position. Only one counter is used
	// instead of tracking block types (for efficiency), and the C++ compiler will catch mismatched block types.
	int blockDepth = 0;

	bool isCurrentLinePreprocessorDirective = false;
	bool isPreprocessorDirectiveAllowedAtCurrentPos = false;

	CppElementIndex() = default;
	CppElementIndex(int64_t pos, int blockDepth, bool isCurrentLinePreprocessorDirective, bool isPreprocessorDirectiveAllowedAtCurrentPos)
		: pos(pos), blockDepth(blockDepth), isCurrentLinePreprocessorDirective(isCurrentLinePreprocessorDirective), isPreprocessorDirectiveAllowedAtCurrentPos(isPreprocessorDirectiveAllowedAtCurrentPos) {
	}

	bool isValid() { return pos >= 0; }
};

StringBuilder & operator<<(StringBuilder &b, CppElementIndex index);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct CppSourceCodeView {
	const uint8_t *data = nullptr;
	int64_t dataSize = 0;
	int64_t startFileOffset = 0;
	CppFile *file = nullptr;

	CppSourceCodeView() = default;
	CppSourceCodeView(const void *data, int64_t dataSize, int64_t startFileOffset, CppFile *file)
		: data((const uint8_t *)data), dataSize(dataSize), startFileOffset(startFileOffset), file(file) {
	}

	bool isEmpty() const { return dataSize <= 0; }
	CppFile * getFile() const;
	int64_t getFileOffsetByPosition(int64_t relativePos) const;
	CppSourceCodeView getSubRange(const CppElementIndex &startIndex, const CppElementIndex &endIndex) const;
	void throwParseExceptionByRelativePos(int64_t relativePos, const string &msg) const;
	void throwParseExceptionAtStart(const string &msg) const;
	void checkRangeValid(int64_t startPos, int64_t endPos) const;
	string getText() const;
	string getText(int64_t startPos, int64_t endPos) const;

	// Returns the source code with comments removed and white-space replaced with single spaces.
	string getSimplifiedCode() const;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class CppSourceCodeBuffer {

	CppSourceCodeView sourceCode;
	void *buffer = nullptr;
	
public:
	CppSourceCodeBuffer(const void *data, int64_t dataSize, int64_t startFileOffset, CppFile *file);
	explicit CppSourceCodeBuffer(const CppSourceCodeView &sourceCode);
	DISABLE_COPY_AND_MOVE(CppSourceCodeBuffer);
	~CppSourceCodeBuffer();

	const CppSourceCodeView & getSourceCode() { return sourceCode; }
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

enum class CppElementType {
	INVALID = 0,
	SPECIAL_CHAR,
	IDENTIFIER,
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct CppElement {
	CppElementIndex startIndex;
	CppElementIndex endIndex;
	CppElementType type = CppElementType::INVALID;
	char ch = 0;
	string text;

	bool isValid() { return type != CppElementType::INVALID; }
	void invalidate();
	string getText();

	void handleIdentifier(const CppElementIndex &startIndex, const CppElementIndex &endIndex, const char *ptr, int64_t length);
	void handleSpecialChar(const CppElementIndex &startIndex, const CppElementIndex &endIndex, char ch);
	void handleEndOfInput(const CppElementIndex &startIndex, const CppElementIndex &endIndex);
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class CppCommentMap {
public:
	struct Entry {
		int64_t startOffset = 0;
		int64_t endOffset = 0;

		Entry(int64_t startOffset, int64_t endOffset) : startOffset(startOffset), endOffset(endOffset) {
		}
	};

	vector<Entry> entries;

	void addCommentIfNotInAlreadyParsedRegion(int64_t startOffset, int64_t endOffset);
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class CppTokenizerTables {
	vector<bool (CppTokenizer::*)(CppElement *resultElement, const CppElementIndex &startIndex, char firstChar)> handlersByCharType;
	bool initialized = false;

public:
	void init();
	void checkInitialized();
	auto getHandlerByCharType(uint8_t type) { return handlersByCharType[type]; }

	static unique_ptr<CppTokenizerTables> instance;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Parses C++ source code into elements.
class CppTokenizer {
	CppSourceCodeView sourceCode;
	CppElementIndex currentIndex;
	CppCommentMap *commentMapIfUsed = nullptr;

	// Prevents the parser from exiting a parentheses, brackets or braces block while allows parsing nested blocks inside it.
	int blockDepthLimit = CPP_INVALID_BLOCK_DEPTH;

	//----------------------------------------------------------------------------------------
	
	char readChar() { return getData()[currentIndex.pos++]; }
	char peekChar() { return getData()[currentIndex.pos]; }

	void skipUntil(const char *s);
	bool trySkipLineBreakAfterBackslash();
	void handleComment(int64_t startOffset, int64_t endOffset);
	bool tryReadCommentAfterFirstSlashRead();
	void skipHorizontalSpaces();
	void skipIdentifierChars();
	void skipUntilPreprocessorDirective(const string &directive);
	// Parameter: the line break should not usually be skipped, because it is handled on the next round and is needed for e.g. preprocessor directive parsing.
	void skipUntilLineBreak(bool shouldSkipLineBreakToo);
	CppStringLiteralType parseStringLiteralTypePrefix(const StringView &prefix);
	bool readStringLiteral(const CppStringLiteralType &type);
	bool readCharLiteral();
	bool readPreprocessorDirective();

	void handleLineBreak() {
		currentIndex.isCurrentLinePreprocessorDirective = false;
		currentIndex.isPreprocessorDirectiveAllowedAtCurrentPos = true;
	}

	// One of these functions is called when the first character of an element is read.
	bool handleChar_unknown(CppElement *resultElement, const CppElementIndex &startIndex, char ch);
	bool handleChar_horizontalSpace(CppElement *resultElement, const CppElementIndex &startIndex, char ch);
	bool handleChar_cr(CppElement *resultElement, const CppElementIndex &startIndex, char ch);
	bool handleChar_lf(CppElement *resultElement, const CppElementIndex &startIndex, char ch);
	bool handleChar_singleQuote(CppElement *resultElement, const CppElementIndex &startIndex, char ch);
	bool handleChar_doubleQuote(CppElement *resultElement, const CppElementIndex &startIndex, char ch);
	bool handleChar_slash(CppElement *resultElement, const CppElementIndex &startIndex, char ch);
	bool handleChar_backslash(CppElement *resultElement, const CppElementIndex &startIndex, char ch);
	bool handleChar_numberSign(CppElement *resultElement, const CppElementIndex &startIndex, char ch);
	bool handleChar_letterOrUnderscore(CppElement *resultElement, const CppElementIndex &startIndex, char ch);
	bool handleChar_digit(CppElement *resultElement, const CppElementIndex &startIndex, char ch);
	bool handleChar_blockStart(CppElement *resultElement, const CppElementIndex &startIndex, char ch);
	bool handleChar_blockEnd(CppElement *resultElement, const CppElementIndex &startIndex, char ch);

public:
	CppTokenizer(const CppSourceCodeView &sourceCode, const CppElementIndex &index, int blockDepthLimit, CppCommentMap *commentMapIfUsed);

	CppSourceCodeView * getSourceCode() { return &sourceCode; }
	const uint8_t * getData() { return getSourceCode()->data; }
	int64_t getDataSize() { return getSourceCode()->dataSize; }
	CppElementIndex getIndex() { return currentIndex; }
	int64_t getRemainingSize() { return getSourceCode()->dataSize - currentIndex.pos; }
	string getDataRangeAsString(int64_t startPos, int64_t endPos);
	int getBlockDepthLimit() { return blockDepthLimit; }
	void setBlockDepthLimit(int limit);
	int setBlockDepthLimitToCurrentBlockAndGetOldLimit();

	bool isFinished() { return currentIndex.pos >= getDataSize(); }
	void moveTo(const CppElementIndex &index) { currentIndex = index; }
	
	// Returns true if an element was read, false if end-of-input reached.
	void readNextElement(CppElement *element);

	void throwParseException(int64_t errorPos, const string &msg);

	friend class CppTokenizerTables;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Groups together a tokenizer that parses C++ source code elements and the state of the current element, for convenience.
struct CppElementIterator {
	CppElement element;
	CppTokenizer tokenizer;

	class InfiniteLoopGuard {
		CppElementIterator *elementIter;
		CppElementIndex startIndex;
	public:
		explicit InfiniteLoopGuard(CppElementIterator *elementIter);
		void checkProgress();
	};

	CppElementIterator(const CppSourceCodeView &sourceCode, const CppElementIndex &index, int blockDepthLimit, CppCommentMap *commentMapIfUsed);
	CppElementIterator(const CppSourceCodeView &sourceCode, CppCommentMap *commentMapIfUsed);
	
	bool isValid() { return element.isValid(); }

	bool moveTo(CppElementIndex index);
	bool moveToNext();
	bool moveToNextAndSkipSubElements();
	CppElementIterator getNextAndSkipSubElements();
	CppSourceCodeView findElementAndGetSourceCodeBeforeIt(bool shouldSkipFoundElement, const function<bool()> &shouldStop);
	CppSourceCodeView * getSourceCode() { return tokenizer.getSourceCode(); }
	SourceLocation getSourceLocation();
	pair<CppElementIndex, CppElementIndex> skipSubElementsAndGetStartAndEndIndex();
	CppSourceCodeView getSubElements();
	CppFile * getFile();
	void throwParseException(const string &msg);
	int64_t getElementStartFileOffset();
	int64_t getElementEndFileOffset();
	int enterBlockAndGetOldLimit();
	void exitBlock(int limit, char endChar, bool lenient);
};

