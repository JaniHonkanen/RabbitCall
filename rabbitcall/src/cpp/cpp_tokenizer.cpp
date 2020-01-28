#include "pch.h"


StringBuilder & operator<<(StringBuilder &b, CppElementIndex index) {
	b << index.pos;
	return b;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CppFile * CppSourceCodeView::getFile() const {
	CHECK_NOT_NULL(file);
	return file;
}

int64_t CppSourceCodeView::getFileOffsetByPosition(int64_t relativePos) const {
	return startFileOffset + clamp(relativePos, (int64_t)0, dataSize);
}

CppSourceCodeView CppSourceCodeView::getSubRange(const CppElementIndex &startIndex, const CppElementIndex &endIndex) const {
	int64_t startPos = startIndex.pos;
	int64_t endPos = endIndex.pos;
	checkRangeValid(startPos, endPos);
	return CppSourceCodeView((uint8_t *)data + startPos, endPos - startPos, startFileOffset + startPos, file);
}

void CppSourceCodeView::throwParseExceptionByRelativePos(int64_t relativePos, const string &msg) const {
	throw ParseException(getFile()->getSourceLocationByFileOffset(getFileOffsetByPosition(relativePos)), msg);
}

void CppSourceCodeView::throwParseExceptionAtStart(const string &msg) const {
	throwParseExceptionByRelativePos(0, msg);
}

void CppSourceCodeView::checkRangeValid(int64_t startPos, int64_t endPos) const {
	if (startPos < 0 || startPos > dataSize) EXC(sb() << "Start index out of range: " << startPos);
	if (endPos < 0 || endPos > dataSize) EXC(sb() << "End index out of range: " << endPos);
	if (startPos > endPos) EXC(sb() << "Start index " << startPos << " should not be greater than end index " << endPos);
}

string CppSourceCodeView::getText() const {
	return string((const char *)data, dataSize);
}

string CppSourceCodeView::getText(int64_t startPos, int64_t endPos) const {
	checkRangeValid(startPos, endPos);
	return string((const char *)data + startPos, endPos - startPos);
}

string CppSourceCodeView::getSimplifiedCode() const {
	string result;
	int64_t previousEndOffset = -1;
	CppElementIterator elementIter(*this, nullptr);
	while (elementIter.isValid()) {
		int64_t startOffset = elementIter.element.startIndex.pos;
		int64_t endOffset = elementIter.element.endIndex.pos;

		// If there are spaces (or line-breaks or comment blocks) between the elements, add a space.
		if (previousEndOffset != -1 && startOffset > previousEndOffset) {
			result.push_back(' ');
		}

		result.append((const char *)data + startOffset, endOffset - startOffset);
		previousEndOffset = endOffset;
		elementIter.moveToNext();
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CppSourceCodeBuffer::CppSourceCodeBuffer(const void *data, int64_t dataSize, int64_t startFileOffset, CppFile *file) {
	buffer = allocateMemory(dataSize);
	memcpy(buffer, data, dataSize);
	sourceCode = CppSourceCodeView(buffer, dataSize, startFileOffset, file);
}

CppSourceCodeBuffer::CppSourceCodeBuffer(const CppSourceCodeView &sourceCode)
	: CppSourceCodeBuffer(sourceCode.data, sourceCode.dataSize, sourceCode.startFileOffset, sourceCode.file) {
}

CppSourceCodeBuffer::~CppSourceCodeBuffer() {
	if (buffer) {
		freeMemory(buffer);
		buffer = nullptr;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CppElement::invalidate() {
	startIndex = CppElementIndex();
	endIndex = CppElementIndex();
	type = CppElementType::INVALID;
	ch = 0;
	text.clear();
}

string CppElement::getText() {
	if (!text.empty()) return text;
	if (ch != '\0') return string(&ch, 1);
	return "";
}

void CppElement::handleIdentifier(const CppElementIndex &startIndex, const CppElementIndex &endIndex, const char *ptr, int64_t length) {
	invalidate();
	type = CppElementType::IDENTIFIER;
	this->startIndex = startIndex;
	this->endIndex = endIndex;
	text = string(ptr, length);
}

void CppElement::handleSpecialChar(const CppElementIndex &startIndex, const CppElementIndex &endIndex, char ch) {
	invalidate();
	type = CppElementType::SPECIAL_CHAR;
	this->startIndex = startIndex;
	this->endIndex = endIndex;
	this->ch = ch;
}

void CppElement::handleEndOfInput(const CppElementIndex &startIndex, const CppElementIndex &endIndex) {
	invalidate();
	this->startIndex = startIndex;
	this->endIndex = endIndex;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CppCommentMap::addCommentIfNotInAlreadyParsedRegion(int64_t startOffset, int64_t endOffset) {
	if (entries.empty() || entries.at(entries.size() - 1).startOffset < startOffset) {
		entries.emplace_back(startOffset, endOffset);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

unique_ptr<CppTokenizerTables> CppTokenizerTables::instance;

void CppTokenizerTables::init() {

	int tableSize = 256;
	handlersByCharType.resize(tableSize);
	for (int i = 0; i < tableSize; i++) {
		handlersByCharType.at(i) = &CppTokenizer::handleChar_unknown;
	}

	auto setCharHandler = [&](CppCharType type, auto handler) {
		handlersByCharType.at((uint8_t)type) = handler;
		handlersByCharType.at((uint8_t)type | (uint8_t)CppCharType::FLAG_OPERATOR_CHAR) = handler;
	};

	setCharHandler(CppCharType::HORIZONTAL_SPACE, &CppTokenizer::handleChar_horizontalSpace);
	setCharHandler(CppCharType::CR, &CppTokenizer::handleChar_cr);
	setCharHandler(CppCharType::LF, &CppTokenizer::handleChar_lf);
	setCharHandler(CppCharType::SINGLE_QUOTE, &CppTokenizer::handleChar_singleQuote);
	setCharHandler(CppCharType::DOUBLE_QUOTE, &CppTokenizer::handleChar_doubleQuote);
	setCharHandler(CppCharType::SLASH, &CppTokenizer::handleChar_slash);
	setCharHandler(CppCharType::BACKSLASH, &CppTokenizer::handleChar_backslash);
	setCharHandler(CppCharType::NUMBER_SIGN, &CppTokenizer::handleChar_numberSign);
	setCharHandler(CppCharType::BLOCK_START, &CppTokenizer::handleChar_blockStart);
	setCharHandler(CppCharType::BLOCK_END, &CppTokenizer::handleChar_blockEnd);

	auto setCharTypeFlagHandler = [&](CppCharType flag, auto handler) {
		for (int baseType = 0; baseType < (int)CppCharType::MAX_TYPES; baseType++) {
			handlersByCharType.at((uint8_t)flag | (uint8_t)baseType) = handler;
		}
	};
	setCharTypeFlagHandler(CppCharType::FLAG_LETTER_OR_UNDERSCORE, &CppTokenizer::handleChar_letterOrUnderscore);
	setCharTypeFlagHandler(CppCharType::FLAG_DIGIT, &CppTokenizer::handleChar_digit);

	initialized = true;
}

void CppTokenizerTables::checkInitialized() {
	if (!initialized) EXC(sb() << "Char type table not initialized");
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CppTokenizer::CppTokenizer(const CppSourceCodeView &sourceCode, const CppElementIndex &index, int blockDepthLimit, CppCommentMap *commentMapIfUsed)
	: sourceCode(sourceCode), currentIndex(index), commentMapIfUsed(commentMapIfUsed), blockDepthLimit(blockDepthLimit) {

	CppTokenizerTables::instance->checkInitialized();

	if (!currentIndex.isValid()) {
		currentIndex = CppElementIndex(0, 0, false, true);
	}
}

string CppTokenizer::getDataRangeAsString(int64_t startPos, int64_t endPos) {
	return string((const char *)getData() + startPos, endPos - startPos);
}

void CppTokenizer::setBlockDepthLimit(int limit) {
	blockDepthLimit = limit;
}

int CppTokenizer::setBlockDepthLimitToCurrentBlockAndGetOldLimit() {
	int oldLimit = getBlockDepthLimit();
	setBlockDepthLimit(currentIndex.blockDepth);
	return oldLimit;
}

void CppTokenizer::skipUntil(const char *s) {
	int64_t length = strlen(s);
	while (getRemainingSize() >= length) {
		if (memcmp(getData() + currentIndex.pos, s, length) == 0) break;
		currentIndex.pos++;
	}
}

bool CppTokenizer::trySkipLineBreakAfterBackslash() {
	if (currentIndex.pos < (int64_t)getDataSize()) {
		char ch = getData()[currentIndex.pos];
		if (ch == '\n') {
			currentIndex.pos++;
			return true;
		}
		else if (ch == '\r' && currentIndex.pos + 1 < (int64_t)getDataSize() && getData()[currentIndex.pos + 1] == '\n') {
			currentIndex.pos += 2;
			return true;
		}
	}
	return false;
}

void CppTokenizer::handleComment(int64_t startOffset, int64_t endOffset) {
	if (commentMapIfUsed) {
		commentMapIfUsed->addCommentIfNotInAlreadyParsedRegion(startOffset, endOffset);
	}
}

bool CppTokenizer::tryReadCommentAfterFirstSlashRead() {
	int64_t startOffset = currentIndex.pos - 1;
	if (!isFinished()) {
		char ch2 = peekChar();
		if (ch2 == '/') { // Single-line comment
			currentIndex.pos++;
			skipUntilLineBreak(false);
			handleComment(startOffset, currentIndex.pos);
			return true;
		}
		else if (ch2 == '*') { // Block comment
			currentIndex.pos++;
			skipUntil("*/");
			currentIndex.pos = min(currentIndex.pos + 2, (int64_t)getDataSize());
			handleComment(startOffset, currentIndex.pos);
			return true;
		}
	}
	return false;
}

void CppTokenizer::skipHorizontalSpaces() {
	while (currentIndex.pos < getDataSize()) {
		if (!cppParseUtil->isHorizontalSpace(getData()[currentIndex.pos])) break;
		currentIndex.pos++;
	}
}

void CppTokenizer::skipIdentifierChars() {
	while (!isFinished()) {
		// Don't check for backslash-line-breaks in the middle of an identifier (very rare and not supported by some tools either).
		char ch = getData()[currentIndex.pos];
		if (!cppParseUtil->isLetterOrUnderscoreOrDigit(ch)) break;
		currentIndex.pos++;
	}	
}

void CppTokenizer::skipUntilPreprocessorDirective(const string &directive) {
	while (!isFinished()) {
		skipUntilLineBreak(true);
		skipHorizontalSpaces();

		if (!isFinished()) {
			char ch = getData()[currentIndex.pos];
			if (ch == '#') {
				currentIndex.pos++; // Increment position only after identifying this as a #, because if this is an empty line, the line-break scanning would skip it and also the next line.
				skipHorizontalSpaces();
				if (!isFinished()) {
					int64_t identifierStartPos = currentIndex.pos;
					char ch2 = getData()[currentIndex.pos++];
					if (cppParseUtil->isLetterOrUnderscore(ch2)) {
						skipIdentifierChars();
						string identifier = getDataRangeAsString(identifierStartPos, currentIndex.pos);
						if (identifier == directive) {
							skipUntilLineBreak(false);
							return;
						}
					}
				}
			}
		}
	}	
}

void CppTokenizer::skipUntilLineBreak(bool shouldSkipLineBreakToo) {
	while (!isFinished()) {
		char ch = readChar();
		if (ch != '\\' || !trySkipLineBreakAfterBackslash()) {
			if (ch == '\n') {
				if (!shouldSkipLineBreakToo) {
					currentIndex.pos--;
				}
				break;
			}
			else if (ch == '\r' && currentIndex.pos + 1 < getDataSize() && getData()[currentIndex.pos + 1] == '\n') {
				if (shouldSkipLineBreakToo) {
					currentIndex.pos++; // Skip the LF, because the CR is already skipped.
				}
				else {
					currentIndex.pos--;
				}
				break;
			}
		}
	}
}

CppStringLiteralType CppTokenizer::parseStringLiteralTypePrefix(const StringView &prefix) {
	CppStringLiteralType type;
	StringView p = prefix;

	if (p.length >= 1 && p.ptr[p.length - 1] == 'R') {
		type.isRaw = true;
		p = StringView(p.ptr, p.length - 1);
		if (p.length == 0) {
			type.isValid = true;
		}
	}

	if (p.length >= 1) {
		if (p == "L" || p == "u8" || p == "u" || p == "U") {
			type.isValid = true;
		}
	}

	if (!type.isValid) type = CppStringLiteralType(); // Clear fields if invalid.
	return type;
}

bool CppTokenizer::readStringLiteral(const CppStringLiteralType &type) {
	bool isRaw = type.isValid && type.isRaw;
	string rawStringDelimiter;
	if (isRaw) {
		int64_t delimiterStart = currentIndex.pos;
		while (!isFinished()) {
			char ch = readChar();
			if (ch == '(') {
				rawStringDelimiter += string((const char *)getData() + delimiterStart, currentIndex.pos - 1 - delimiterStart);
				rawStringDelimiter += "\"";
				break;
			}
			if (ch == '\"') return false;
		}
		//LOG_DEBUG(sb() << "Raw string delimiter: " << rawStringDelimiter);
	}
	
	// Scan until a closing quote if found so that also raw string literals are skipped.
	// A non-raw string literal should not continue until a line-break (unless it is preceded by a backslash),
	// but don't raise an error if it does (the compiler should).
	while (!isFinished()) {
		char ch = readChar();
		if (isRaw) {
			if (ch == ')' && getRemainingSize() >= (int64_t)rawStringDelimiter.size() && memcmp(rawStringDelimiter.data(), getData() + currentIndex.pos, rawStringDelimiter.size()) == 0) {
				//LOG_DEBUG(sb() << "Raw string: " << string((const char *)getData() + startPos, currentIndex.pos - startPos - 1));
				currentIndex.pos += rawStringDelimiter.size();
				break;
			}
		}
		else {
			if (ch == '\\') {
				if (trySkipLineBreakAfterBackslash()) {
				}
				else if (!isFinished()) {
					if (peekChar() == '\"') { // Escaped quote.
						currentIndex.pos++;
					}
					else if (peekChar() == '\\') { // Escaped backslash.
						currentIndex.pos++;
					}
				}
			}
			else if (ch == '\r' || ch == '\n') {
				// Line-breaks are not allowed in the middle of a string literal, but there is some code that has e.g. erroneous string literals inside #if blocks
				// that are only visited in error situations and have therefore gone undetected => be lenient and interpret that the string ends at the line-break.
				currentIndex.pos--;
				break;
			}
			else if (ch == '\"') {
				break;
			}
		}
	}
	return false; // String literals are currently only skipped, not handled as elements.
}

bool CppTokenizer::readCharLiteral() {
	while (!isFinished()) {
		char ch = readChar();
		if (ch == '\\') { // Escaped char (can be also another backslash or a quote).
			currentIndex.pos++;
		}
		else if (ch == '\'') {
			break;
		}
	}
	return false; // Char literal not currently handled as an element.
}

bool CppTokenizer::readPreprocessorDirective() {
	skipHorizontalSpaces();
	if (!isFinished()) {
		int64_t identifierStartPos = currentIndex.pos;
		char ch2 = getData()[currentIndex.pos];
		if (cppParseUtil->isLetterOrUnderscore(ch2)) {
			currentIndex.pos++;
			skipIdentifierChars();
			string identifier = getDataRangeAsString(identifierStartPos, currentIndex.pos);

			if (identifier == "else" || identifier == "elif") {
				// Skip #else and #elif blocks because they might contain e.g. opening braces that should be alternative to the one defined in the main #if block,
				// but the parser would interpret both of them and cause errors.
				skipUntilPreprocessorDirective("endif");
			}
			else {
				currentIndex.isCurrentLinePreprocessorDirective = true;
			}
		}
	}
	return false; // Preprocessor directive not currently handled as an element.
}

bool CppTokenizer::handleChar_unknown(CppElement *resultElement, const CppElementIndex &startIndex, char ch) {
	if (currentIndex.isCurrentLinePreprocessorDirective) return false;
	currentIndex.isPreprocessorDirectiveAllowedAtCurrentPos = false;
	resultElement->handleSpecialChar(startIndex, currentIndex, ch);
	return true;
}

bool CppTokenizer::handleChar_horizontalSpace(CppElement *resultElement, const CppElementIndex &startIndex, char ch) {
	skipHorizontalSpaces();
	return false;
}

bool CppTokenizer::handleChar_cr(CppElement *resultElement, const CppElementIndex &startIndex, char ch) {
	if (!isFinished() && peekChar() == '\n') {
		currentIndex.pos++;
		handleLineBreak();
	}
	return false;
}

bool CppTokenizer::handleChar_lf(CppElement *resultElement, const CppElementIndex &startIndex, char ch) {
	handleLineBreak();
	return false;
}

bool CppTokenizer::handleChar_singleQuote(CppElement *resultElement, const CppElementIndex &startIndex, char ch) {
	currentIndex.isPreprocessorDirectiveAllowedAtCurrentPos = false;
	return readCharLiteral();
}

bool CppTokenizer::handleChar_doubleQuote(CppElement *resultElement, const CppElementIndex &startIndex, char ch) {
	currentIndex.isPreprocessorDirectiveAllowedAtCurrentPos = false;
	return readStringLiteral(CppStringLiteralType());
}

bool CppTokenizer::handleChar_slash(CppElement *resultElement, const CppElementIndex &startIndex, char ch) {
	currentIndex.isPreprocessorDirectiveAllowedAtCurrentPos = false;
	if (tryReadCommentAfterFirstSlashRead()) {
		return false;
	}
	return handleChar_unknown(resultElement, startIndex, ch); // The slash was not a part of a comment.
}

bool CppTokenizer::handleChar_backslash(CppElement *resultElement, const CppElementIndex &startIndex, char ch) {
	if (trySkipLineBreakAfterBackslash()) {
		// Backslash followed by line-break => don't consider it an actual line-break (should not make much difference in this context (outside string literals),
		// except don't allow preprocessor directives on the next line).
		return false;
	}
	return handleChar_unknown(resultElement, startIndex, ch);
}

bool CppTokenizer::handleChar_numberSign(CppElement *resultElement, const CppElementIndex &startIndex, char ch) {
	if (currentIndex.isCurrentLinePreprocessorDirective) return false;
	if (currentIndex.isPreprocessorDirectiveAllowedAtCurrentPos) {
		currentIndex.isPreprocessorDirectiveAllowedAtCurrentPos = false;
		return readPreprocessorDirective();
	}
	return handleChar_unknown(resultElement, startIndex, ch);
}

bool CppTokenizer::handleChar_letterOrUnderscore(CppElement *resultElement, const CppElementIndex &startIndex, char ch) {
	if (currentIndex.isCurrentLinePreprocessorDirective) return false;
	currentIndex.isPreprocessorDirectiveAllowedAtCurrentPos = false;
	int64_t startPos = currentIndex.pos - 1;
	skipIdentifierChars();

	const char *startPtr = (const char *)getData() + startPos;
	int64_t length = currentIndex.pos - startPos;
	
	if (!isFinished() && peekChar() == '\"') {
		CppStringLiteralType type = parseStringLiteralTypePrefix(StringView(startPtr, length));
		if (type.isValid) {
			readChar(); // Skip the first quote.
			return readStringLiteral(type);
		}
	}
	
	resultElement->handleIdentifier(startIndex, currentIndex, startPtr, length);
	return true;
}

bool CppTokenizer::handleChar_digit(CppElement *resultElement, const CppElementIndex &startIndex, char ch) {
	if (currentIndex.isCurrentLinePreprocessorDirective) return false;
	currentIndex.isPreprocessorDirectiveAllowedAtCurrentPos = false;
	return handleChar_unknown(resultElement, startIndex, ch);
}

bool CppTokenizer::handleChar_blockStart(CppElement *resultElement, const CppElementIndex &startIndex, char ch) {
	if (currentIndex.isCurrentLinePreprocessorDirective) return false;
	currentIndex.isPreprocessorDirectiveAllowedAtCurrentPos = false;
	currentIndex.blockDepth++;
	resultElement->handleSpecialChar(startIndex, currentIndex, ch);
	return true;
}

bool CppTokenizer::handleChar_blockEnd(CppElement *resultElement, const CppElementIndex &startIndex, char ch) {
	if (currentIndex.isCurrentLinePreprocessorDirective) return false;
	currentIndex.isPreprocessorDirectiveAllowedAtCurrentPos = false;

	if (blockDepthLimit != CPP_INVALID_BLOCK_DEPTH && currentIndex.blockDepth <= blockDepthLimit) {
		currentIndex.pos--;
		resultElement->handleEndOfInput(startIndex, currentIndex);
	}
	else {
		currentIndex.blockDepth = max(0, currentIndex.blockDepth - 1);
		resultElement->handleSpecialChar(startIndex, currentIndex, ch);
	}
	return true;
}

void CppTokenizer::readNextElement(CppElement *element) {
	if (currentIndex.pos < 0) getSourceCode()->getFile()->throwParseExceptionByFileOffset(getSourceCode()->startFileOffset, sb() << "Tokenizer position out of range: " << currentIndex.pos);
	while (true) {
		if (isFinished()) {
			element->handleEndOfInput(currentIndex, currentIndex);
			break;
		}
		CppElementIndex startIndex = currentIndex;
		char ch = readChar();
		uint8_t charType = cppParseUtil->getCharType(ch);
		if ((this->*CppTokenizerTables::instance->getHandlerByCharType(charType))(element, startIndex, ch)) {
			break;
		}
	}
}

void CppTokenizer::throwParseException(int64_t errorPos, const string &msg) {
	getSourceCode()->throwParseExceptionByRelativePos(errorPos, msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CppElementIterator::InfiniteLoopGuard::InfiniteLoopGuard(CppElementIterator *elementIter)
	: elementIter(elementIter) {
	startIndex = elementIter->element.startIndex;
}

void CppElementIterator::InfiniteLoopGuard::checkProgress() {
	if (elementIter->isValid()) {
		CppElementIndex currentIndex = elementIter->element.startIndex;
		if (currentIndex.pos <= startIndex.pos) elementIter->throwParseException(sb() << "Element iterator did not proceed forward in loop, was " << startIndex << ", now " << currentIndex);
	}
}

CppElementIterator::CppElementIterator(const CppSourceCodeView &sourceCode, const CppElementIndex &index, int blockDepthLimit, CppCommentMap *commentMapIfUsed)
	: tokenizer(sourceCode, index, blockDepthLimit, commentMapIfUsed) {

	moveToNext();
}

CppElementIterator::CppElementIterator(const CppSourceCodeView &sourceCode, CppCommentMap *commentMapIfUsed)
	: CppElementIterator(sourceCode, CppElementIndex(), CPP_INVALID_BLOCK_DEPTH, commentMapIfUsed) {
}

bool CppElementIterator::moveTo(CppElementIndex index) {
	element.invalidate();
	tokenizer.moveTo(index);
	return moveToNext();
}

bool CppElementIterator::moveToNext() {
	tokenizer.readNextElement(&element);
	return element.isValid();
}

bool CppElementIterator::moveToNextAndSkipSubElements() {
	if (element.ch == '(' || element.ch == '[' || element.ch == '{') {
		skipSubElementsAndGetStartAndEndIndex();
	}
	return moveToNext();
}

CppElementIterator CppElementIterator::getNextAndSkipSubElements() {
	CppElementIterator newIter = *this;
	newIter.moveToNextAndSkipSubElements();
	return newIter;
}

CppSourceCodeView CppElementIterator::findElementAndGetSourceCodeBeforeIt(bool shouldSkipFoundElement, const function<bool()> &shouldStop) {
	CppElementIndex startIndex = element.startIndex;
	while (true) {
		if (!isValid()) {
			return getSourceCode()->getSubRange(startIndex, tokenizer.getIndex());
		}
		if (shouldStop()) {
			CppSourceCodeView result = getSourceCode()->getSubRange(startIndex, element.startIndex);
			if (shouldSkipFoundElement) moveToNextAndSkipSubElements();
			return result;
		}
		moveToNextAndSkipSubElements();
	}
}

SourceLocation CppElementIterator::getSourceLocation() {
	return getFile()->getSourceLocationByFileOffset(getElementStartFileOffset());
}

pair<CppElementIndex, CppElementIndex> CppElementIterator::skipSubElementsAndGetStartAndEndIndex() {
	char startChar = element.ch;
	char endChar = 0;
	switch (startChar) {
	case '(': endChar = ')'; break;
	case '[': endChar = ']'; break;
	case '{': endChar = '}'; break;
	default:
		throwParseException(sb() << "Cannot get sub-elements for element '" << startChar << "'");
	}

	moveToNext();
	CppElementIndex startIndex = element.startIndex;
	while (isValid()) {
		if (element.ch == endChar) {
			return pair<CppElementIndex, CppElementIndex>(startIndex, element.startIndex);
		}
		moveToNextAndSkipSubElements();
	}
	throwParseException(sb() << "Unclosed '" << startChar << "'");
	return pair<CppElementIndex, CppElementIndex>(); // Not reached.
}

CppSourceCodeView CppElementIterator::getSubElements() {
	CppElementIterator iter = *this;
	pair<CppElementIndex, CppElementIndex> startAndEnd = iter.skipSubElementsAndGetStartAndEndIndex();
	return getSourceCode()->getSubRange(startAndEnd.first, startAndEnd.second);
}

CppFile * CppElementIterator::getFile() {
	return tokenizer.getSourceCode()->getFile();
}

void CppElementIterator::throwParseException(const string &msg) {
	throw ParseException(getSourceLocation(), msg);
}

int64_t CppElementIterator::getElementStartFileOffset() {
	return getSourceCode()->getFileOffsetByPosition(element.startIndex.pos);
}

int64_t CppElementIterator::getElementEndFileOffset() {
	return getSourceCode()->getFileOffsetByPosition(element.endIndex.pos);
}

int CppElementIterator::enterBlockAndGetOldLimit() {
	int oldLimit = tokenizer.setBlockDepthLimitToCurrentBlockAndGetOldLimit();
	moveToNext();
	return oldLimit;
}

void CppElementIterator::exitBlock(int limit, char endChar, bool lenient) {
	tokenizer.setBlockDepthLimit(limit);
	moveToNext();

	// A missing end-of-block may be allowed at the end of file, because partial files or e.g. unsupported ifdefs could contain unclosed braces.
	if (!lenient || element.isValid()) {
		if (!element.isValid() || element.ch != endChar) throwParseException(sb() << "Expected '" << endChar << "', got: " << element.getText());
	}

	moveToNext();
}


