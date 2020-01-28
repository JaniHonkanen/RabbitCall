#include "pch.h"


CppParsedFile::CppParsedFile(const shared_ptr<CppFile> &file)
	: file(file) {
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

shared_ptr<CppExportParameters> CppFileParser::parseExportParameters(CppElementIterator &elementIter) {
	shared_ptr<CppExportParameters> result = make_shared<CppExportParameters>();

	if (!elementIter.isValid() || elementIter.element.ch != '(') elementIter.throwParseException(sb() << "Expected parameters after export keyword " << config->exportKeywordWithParameters << " (use " << config->exportKeyword << " instead if you did not intend to give parameters)");
	unordered_map<string, CppSourceCodeView> parameters = CppItemParseUtil::parseCommaSeparatedParameterMap(elementIter.getSubElements());
	elementIter.moveToNextAndSkipSubElements();

	string hlslParameterName = "hlsl";
	if (contains(parameters, hlslParameterName)) {
		result->hlslParameters.exportHlsl = true;

		unordered_map<string, CppSourceCodeView> p = CppItemParseUtil::parseCommaSeparatedParameterMap(parameters.at(hlslParameterName));

		string cbufferParameterName = "cbuffer";
		if (contains(p, cbufferParameterName)) result->hlslParameters.isCBuffer = true;

		string prefixParameterName = "prefix";
		if (contains(p, prefixParameterName)) result->hlslParameters.fieldPrefix = p.at(prefixParameterName).getText();

		string registerParameterName = "register";
		if (contains(p, registerParameterName)) result->hlslParameters.registerName = p.at(registerParameterName).getText();
	}

	string glslParameterName = "glsl";
	if (contains(parameters, glslParameterName)) {
		result->glslParameters.exportGlsl = true;

		unordered_map<string, CppSourceCodeView> p = CppItemParseUtil::parseCommaSeparatedParameterMap(parameters.at(glslParameterName));

		string storageParameterName = "storage";
		if (contains(p, storageParameterName)) result->glslParameters.storage = p.at(storageParameterName).getText();

		string instanceNameParameterName = "instanceName";
		if (contains(p, instanceNameParameterName)) result->glslParameters.instanceName = p.at(instanceNameParameterName).getText();

		string prefixParameterName = "prefix";
		if (contains(p, prefixParameterName)) result->glslParameters.fieldPrefix = p.at(prefixParameterName).getText();

		string bindingParameterName = "binding";
		if (contains(p, bindingParameterName)) result->glslParameters.binding = p.at(bindingParameterName).getText();
	}

	return result;
}

string CppFileParser::getLineText(int64_t lineNumber) {
	int64_t startOffset = file->lineAndColumnNumberMap->getLineStartOffsetByLineNumber(lineNumber);
	int64_t endOffset = file->lineAndColumnNumberMap->getLineStartOffsetByLineNumber(lineNumber + 1);

	if (endOffset > startOffset && (*(content.getBuffer() + endOffset - 1)) == '\n') {
		endOffset--;
		if (endOffset > startOffset && (*(content.getBuffer() + endOffset - 1)) == '\r') {
			endOffset--;
		}
	}

	return string((const char *)(content.getBuffer() + startOffset), max((int64_t)0, endOffset - startOffset));
}

bool CppFileParser::isLineAcceptedBetweenCommentAndDeclaration(int64_t lineNumber) {
	const char *data = (const char *)content.getBuffer();
	int64_t dataSize = content.getPosition();
	int64_t i = file->lineAndColumnNumberMap->getLineStartOffsetByLineNumber(lineNumber);
	for (; i < dataSize; i++) {
		if (!cppParseUtil->isHorizontalSpace(data[i])) break;
	}

	if (i + 2 <= dataSize && data[i] == '[' && data[i + 1] == '[') { // C++ attribute, e.g. [[nodiscard]].
		return true;
	}
	return false;
}

shared_ptr<CppComment> CppFileParser::parseCommentLinesAboveDeclaration(int64_t declarationOffset) {
	LineAndColumnNumberMap *lineAndColumnNumberMap = file->lineAndColumnNumberMap.get();
	shared_ptr<CppComment> result = make_shared<CppComment>();
	if (!commentMap->entries.empty()) {
		// Check if there are attributes such as [[nodiscard]] on lines between the comment and the declaration and skip them.
		int64_t startLineNumber = lineAndColumnNumberMap->getLineAndColumnNumberByOffset(declarationOffset).line;
		while (startLineNumber > 1) {
			if (!isLineAcceptedBetweenCommentAndDeclaration(startLineNumber - 1)) {
				break;
			}
			startLineNumber--;
		}
		int64_t startOffset = lineAndColumnNumberMap->getLineStartOffsetByLineNumber(startLineNumber);

		// Find the next last comment before the declaration (lower_bound returns the next comment after that) and scan comments backwards until code elements or an empty line is found.
		int64_t commentIndex = lower_bound(commentMap->entries.begin(), commentMap->entries.end(), CppCommentMap::Entry(0, startOffset), [](const CppCommentMap::Entry &e1, const CppCommentMap::Entry &e2) -> bool { return e1.endOffset < e2.endOffset; }) - commentMap->entries.begin();
		int64_t declarationOrNextCommentOffset = startOffset;
		int64_t endCommentIndex = commentIndex;
		while (commentIndex > 0) {
			int64_t declarationOrNextCommentLineNumber = lineAndColumnNumberMap->getLineAndColumnNumberByOffset(declarationOrNextCommentOffset).line;
			CppCommentMap::Entry *commentEntry = &commentMap->entries.at(commentIndex - 1);
			int64_t commentEndLineNumber = lineAndColumnNumberMap->getLineAndColumnNumberByOffset(commentEntry->endOffset).line;

			if (commentEndLineNumber < declarationOrNextCommentLineNumber - 1) {
				break;
			}

			int64_t commentStartLineNumber = lineAndColumnNumberMap->getLineAndColumnNumberByOffset(commentEntry->startOffset).line;
			int64_t commentStartLineStartOffset = lineAndColumnNumberMap->getLineStartOffsetByLineNumber(commentStartLineNumber);
			int64_t declarationOrNextCommentLineStartOffset = lineAndColumnNumberMap->getLineStartOffsetByLineNumber(declarationOrNextCommentLineNumber);

			// Don't accept the comment if there are non-whitespace characters before or after the comment on the same line
			// (regardless of whether it is a single-line or multi-line comment).
			if (!cppParseUtil->containsWhiteSpaceOnly((const char*)(content.getBuffer() + commentStartLineStartOffset), max((int64_t)0, commentEntry->startOffset - commentStartLineStartOffset)) ||
				!cppParseUtil->containsWhiteSpaceOnly((const char *)(content.getBuffer() + commentEntry->endOffset), max((int64_t)0, declarationOrNextCommentLineStartOffset - commentEntry->endOffset))) {
				break;
			}

			declarationOrNextCommentOffset = commentEntry->startOffset;
			commentIndex--;
		}

		for (int64_t index = commentIndex; index < endCommentIndex; index++) {
			CppCommentMap::Entry *commentEntry = &commentMap->entries.at(index);
			int64_t firstLineNumber = lineAndColumnNumberMap->getLineAndColumnNumberByOffset(commentEntry->startOffset).line;
			int64_t lastLineNumber = lineAndColumnNumberMap->getLineAndColumnNumberByOffset(commentEntry->endOffset).line;

			// Get indentation from the first comment line and then remove the same identation from subsequent lines.
			// Multi-line comments may have more indentation on subsequent lines and this will align them correctly in the generated output
			// (unless tabs and spaces are mixed differently on different lines).
			string indent;
			for (int64_t line = firstLineNumber; line <= lastLineNumber; line++) {
				string text = getLineText(line);
				if (line == firstLineNumber) {
					for (int64_t i = 0; i < (int64_t)text.size(); i++) {
						char ch = text[i];
						if (!cppParseUtil->isHorizontalSpace(ch)) {
							break;
						}
						indent.push_back(ch);
					}
				}

				int64_t i = 0;
				for (; i < (int64_t)text.size(); i++) {
					if (i >= (int64_t)indent.size() || text[i] != indent[i]) {
						break;
					}
				}
				result->lines.push_back(text.substr(i));
			}
		}
	}
	return result;
}

void CppFileParser::parseSuperclassListElement(CppElementIterator &elementIter, CppAccessModifier defaultAccessModifier, const function<void(const CppSuperclassRef &)> &resultCallback) {
	CppSuperclassRef result;
	result.sourceLocation = elementIter.getSourceLocation();
	result.accessModifier = defaultAccessModifier;
	CppElementIndex startIndex = elementIter.element.startIndex;
	while (elementIter.isValid()) {
		CppElementIterator::InfiniteLoopGuard loopGuard(&elementIter);
		
		if (elementIter.element.ch == '[') { // Skip attributes
			elementIter.moveToNextAndSkipSubElements();
		}
		else {
			if (elementIter.element.type != CppElementType::IDENTIFIER) {
				break;
			}

			if (elementIter.element.text == "virtual") elementIter.throwParseException("Virtual base classes not supported");

			CppAccessModifier accessModifier = CppItemParseUtil::parseAccessModifier(elementIter.element.text);
			if (accessModifier != CppAccessModifier::UNKNOWN) {
				result.accessModifier = accessModifier;
				elementIter.moveToNext();
			}
			else {
				// If multiple text strings are encountered, assume the last one is the class name and the others before it are unknown keywords.
				result.partialName = CppItemParseUtil::tryParsePossiblyQualifiedName(elementIter);
				if (result.partialName.empty()) {
					// Skip current element if it was not a name.
					elementIter.moveToNext();
				}
			}
		}

		loopGuard.checkProgress();
	}

	if (result.partialName.empty()) elementIter.getSourceCode()->throwParseExceptionByRelativePos(startIndex.pos, "Could not parse base class name");
	resultCallback(result);
}

void CppFileParser::parseSuperclassList(CppElementIterator &elementIter, CppAccessModifier defaultAccessModifier, const function<void(const CppSuperclassRef &)> &resultCallback) {
	while (elementIter.isValid()) {
		CppElementIterator::InfiniteLoopGuard loopGuard(&elementIter);

		parseSuperclassListElement(elementIter, defaultAccessModifier, resultCallback);

		if (elementIter.isValid() && elementIter.element.ch == ',') {
			elementIter.moveToNext();
		}
		else {
			break;
		}

		loopGuard.checkProgress();
	}
}

CppSourceCodeView CppFileParser::extractFunctionOrVariableDeclaration(CppElementIterator &elementIter) {
	CppElementIndex startIndex = elementIter.element.startIndex;
	CppElementIndex endIndex = startIndex;
	while (elementIter.isValid()) {
		CppElementIterator::InfiniteLoopGuard loopGuard(&elementIter);

		if (elementIter.element.ch == '{') {
			endIndex = elementIter.element.startIndex;
			elementIter.moveToNextAndSkipSubElements();

			// Skip the optional semicolon after function body.
			if (elementIter.isValid() && elementIter.element.ch == ';') {
				elementIter.moveToNext();
			}
			break;
		}

		if (elementIter.element.ch == ';') {
			endIndex = elementIter.element.startIndex;
			elementIter.moveToNext();
			break;
		}

		elementIter.moveToNextAndSkipSubElements();
		endIndex = elementIter.element.startIndex; // Update the end index here so that if the input ends, the declaration will still be accepted.

		loopGuard.checkProgress();
	}
	return elementIter.getSourceCode()->getSubRange(startIndex, endIndex);
}

bool CppFileParser::tryParseClass(CppElementIterator &elementIter, const string &namespacePrefix, CppClass *enclosingClassIfExists, shared_ptr<CppExportParameters> exportParameters) {
	CppElementIterator startElement = elementIter;
	while (elementIter.isValid()) {
		CppElementIterator::InfiniteLoopGuard loopGuard(&elementIter);

		if (elementIter.element.ch == '{' || elementIter.element.ch == ';') {
			// End of declaration reached before finding a class/struct/enum keyword.
			elementIter.moveToNextAndSkipSubElements();
			break;
		}

		if (elementIter.element.type == CppElementType::IDENTIFIER) {
			CppClassDeclarationType declType = CppClassDeclarationType::UNKNOWN;
			string itemType = elementIter.element.text;
			if (itemType == "class") declType = CppClassDeclarationType::CLASS;
			if (itemType == "struct") declType = CppClassDeclarationType::STRUCT;
			if (itemType == "enum") declType = CppClassDeclarationType::ENUM;
			if (itemType == "union") elementIter.throwParseException("Union not supported");
			bool isClassOrStruct = (declType == CppClassDeclarationType::CLASS || declType == CppClassDeclarationType::STRUCT);

			if (declType != CppClassDeclarationType::UNKNOWN) {
				if (enclosingClassIfExists) elementIter.throwParseException(sb() << "Nested exported classes not supported, enclosing class: " << enclosingClassIfExists->typeNames.cppType);

				shared_ptr<CppComment> comment = parseCommentLinesAboveDeclaration(elementIter.getElementStartFileOffset());

				SourceLocation sourceLocation = elementIter.getSourceLocation();
				elementIter.moveToNext();

				// Check if this is an enum with a "class" keyword, i.e. "enum class MyEnum"
				bool isClassEnum = false;
				if (declType == CppClassDeclarationType::ENUM) {
					if (elementIter.isValid() && elementIter.element.text == "class") {
						isClassEnum = true;
						elementIter.moveToNext();
					}
				}

				CppAccessModifier defaultAccessModifier = (declType == CppClassDeclarationType::CLASS) ? CppAccessModifier::PRIVATE : CppAccessModifier::PUBLIC;

				int64_t explicitAlignment = 0;
				if (isClassOrStruct) {
					if (elementIter.isValid() && elementIter.element.text == "alignas") {
						elementIter.moveToNext();
						explicitAlignment = CppItemParseUtil::parseAlignasParameter(elementIter);
					}
				}

				if (!elementIter.isValid() || elementIter.element.type != CppElementType::IDENTIFIER) elementIter.throwParseException("Expected class/struct/enum name");
				string unqualifiedClassName = elementIter.element.text;
				elementIter.moveToNext();

				string qualifiedClassName = namespacePrefix + unqualifiedClassName;

				LOG_DEBUG(sb() << "Parsing " << itemType << ": " << qualifiedClassName);
				shared_ptr<CppClass> clazz = make_shared<CppClass>();
				clazz->typeNames.cppType = qualifiedClassName;
				parsedFile->classes.push_back(clazz);
				clazz->isPassByValue = (declType == CppClassDeclarationType::ENUM || declType == CppClassDeclarationType::STRUCT);
				clazz->typeNames.csType = cppParseUtil->cppNamespaceParser.convertToSeparator(namespacePrefix, ".")
					+ ((isClassOrStruct && !clazz->isPassByValue) ? (sb() << config->csClassNamePrefix << unqualifiedClassName << config->csClassNameSuffix) : unqualifiedClassName);
				clazz->typeNames.hlslType = unqualifiedClassName;
				clazz->typeNames.glslType = unqualifiedClassName;
				clazz->comment = comment;
				clazz->sourceLocation = sourceLocation;
				clazz->classType = declType;
				clazz->isClassEnum = isClassEnum;
				clazz->exportParameters = exportParameters;
				clazz->explicitAlignment = explicitAlignment;

				bool baseClassListFound = false;
				while (elementIter.isValid()) {
					CppElementIterator::InfiniteLoopGuard innerLoopGuard(&elementIter);
					
					if (!baseClassListFound && elementIter.element.ch == ':') {
						elementIter.moveToNext();

						baseClassListFound = true;
						parseSuperclassList(elementIter, defaultAccessModifier, [&](const CppSuperclassRef &superclass) {
							LOG_DEBUG(sb() << "Class " << qualifiedClassName << " superclass: " << superclass.partialName);
							clazz->superclasses.push_back(superclass);
						});
					}
					else if (elementIter.element.ch == '{') {
						if (declType == CppClassDeclarationType::CLASS) {
							int oldBlockDepthLimit = elementIter.enterBlockAndGetOldLimit();
							scanForExportDeclarationsSub(elementIter, qualifiedClassName + "::", clazz.get(), false);
							elementIter.exitBlock(oldBlockDepthLimit, '}', false);
						}
						else if (declType == CppClassDeclarationType::STRUCT) {
							int oldBlockDepthLimit = elementIter.enterBlockAndGetOldLimit();
							parseStructBody(elementIter, clazz.get());
							elementIter.exitBlock(oldBlockDepthLimit, '}', false);
						}
						else if (declType == CppClassDeclarationType::ENUM) {
							int oldBlockDepthLimit = elementIter.enterBlockAndGetOldLimit();
							parseEnumBody(elementIter, clazz.get());
							elementIter.exitBlock(oldBlockDepthLimit, '}', false);
						}
						else {
							elementIter.moveToNextAndSkipSubElements();
						}
						break;
					}
					else {
						elementIter.moveToNextAndSkipSubElements();
					}

					innerLoopGuard.checkProgress();
				}

				elementIter.findElementAndGetSourceCodeBeforeIt(true, [&] { return elementIter.element.ch == ';'; });
				return true;
			}
		}

		elementIter.moveToNextAndSkipSubElements();
		
		loopGuard.checkProgress();
	}

	elementIter = startElement;
	return false;
}

void CppFileParser::parseStructBody(CppElementIterator &elementIter, CppClass *classDecl) {
	CHECK_NOT_NULL(classDecl);
	//CppAccessModifier currentAccessModifier = CppAccessModifier::PUBLIC;

	while (elementIter.isValid()) {
		CppElementIterator::InfiniteLoopGuard loopGuard(&elementIter);

		if (elementIter.element.type == CppElementType::IDENTIFIER) {
			string text = elementIter.element.text;

			CppAccessModifier accessModifier = CppItemParseUtil::parseAccessModifier(text);
			if (accessModifier != CppAccessModifier::UNKNOWN) {
				elementIter.moveToNext();
				if (!elementIter.isValid() || elementIter.element.ch != ':') elementIter.throwParseException("Expected ':' after public/protected/private access modifier.");
				elementIter.moveToNext();
				//currentAccessModifier = accessModifier;
			}
			else if (text == "class" || text == "struct" || text == "enum" || text == "typedef") {
				elementIter.moveToNext();
				// Skip the unsupported nested class declaration.
				elementIter.findElementAndGetSourceCodeBeforeIt(true, [&] { return elementIter.element.ch == ';'; });
			}
			else if (text == exportKeyword || text == exportKeywordWithParameters) {
				elementIter.throwParseException("Export keyword not supported inside a data struct.");
			}
			else {
				CppElementIterator declStartElement = elementIter;
				CppSourceCodeView declElementRange = extractFunctionOrVariableDeclaration(elementIter);
				if (!declElementRange.isEmpty()) {
					// Parse the declaration later because some type names that are not known yet might be needed for parsing variable/member/parameter/return types.
					classDecl->unresolvedMembers.emplace_back();
					CppUnresolvedMember &member = classDecl->unresolvedMembers.back();
					member.sourceCode = make_shared<CppSourceCodeBuffer>(declElementRange);
					member.comment = parseCommentLinesAboveDeclaration(declStartElement.getElementStartFileOffset());
				}
				else {
					// If the function/variable parsing failed, ensure that the iterator proceeds.
					elementIter = declStartElement.getNextAndSkipSubElements();
				}
			}
		}
		else if (elementIter.element.ch == '~') {
			// Assume this is a destructor and skip it.
			elementIter.moveToNext();
			elementIter.findElementAndGetSourceCodeBeforeIt(true, [&] { return elementIter.element.ch == ';'; });
		}
		else {
			elementIter.throwParseException(sb() << "Expected class/struct member specification: " << elementIter.element.getText());
		}

		loopGuard.checkProgress();
	}
}

void CppFileParser::parseEnumBody(CppElementIterator &elementIter, CppClass *classDecl) {
	CHECK_NOT_NULL(classDecl);
	while (elementIter.isValid()) {
		CppElementIterator::InfiniteLoopGuard loopGuard(&elementIter);

		CppEnumField field;

		field.comment = parseCommentLinesAboveDeclaration(elementIter.getElementStartFileOffset());
		field.sourceLocation = elementIter.getSourceLocation();
		if (elementIter.element.type != CppElementType::IDENTIFIER) elementIter.throwParseException("Expected enum field name");
		field.name = elementIter.element.text;
		elementIter.moveToNext();
		LOG_DEBUG(sb() << "Parsing enum field: " << field.name);

		// If the content ends here, the enum field is still valid (has no initializer).
		if (elementIter.isValid()) {
			if (elementIter.element.ch == '=') {
				elementIter.moveToNext();

				if (!elementIter.isValid()) elementIter.throwParseException("Expected enum field value");
				int64_t valueStartOffset = elementIter.getElementStartFileOffset();
				int64_t valueEndOffset = valueStartOffset;
				while (elementIter.isValid()) {
					if (elementIter.element.ch == ',') {
						elementIter.moveToNext();
						break;
					}
					valueEndOffset = elementIter.getElementEndFileOffset();
					elementIter.moveToNextAndSkipSubElements();
				}

				field.value = string((const char *)content.getBuffer() + valueStartOffset, valueEndOffset - valueStartOffset);
			}
			else if (elementIter.element.ch == ',') {
				elementIter.moveToNext();
			}
			else {
				elementIter.throwParseException("Expected '=' or ','");
			}
		}

		LOG_DEBUG(sb() << "Enum " << classDecl->typeNames.cppType << " field " << field.name << " = " << field.value);
		classDecl->enumFields.push_back(field);

		loopGuard.checkProgress();
	}
}

void CppFileParser::parseExportDeclaration(CppElementIterator &elementIter, const string &namespacePrefix, CppClass *enclosingClassIfExists, shared_ptr<CppExportParameters> exportParameters, bool insideUnexportedClass) {
	CppElementIndex startIndex = elementIter.element.startIndex;

	if (tryParseClass(elementIter, namespacePrefix, enclosingClassIfExists, exportParameters)) {
		return;
	}

	// The declaration was not a class => try to parse it as a function.
	int64_t functionDeclStartOffset = elementIter.getElementStartFileOffset();
	CppSourceCodeView functionDeclRange = extractFunctionOrVariableDeclaration(elementIter);
	if (functionDeclRange.isEmpty()) elementIter.getSourceCode()->throwParseExceptionByRelativePos(startIndex.pos, "Expected class or function");

	if (insideUnexportedClass) {
		functionDeclRange.throwParseExceptionAtStart("Cannot export a member function without exporting the enclosing class");
	}

	// Parse the declaration later because some type names that are not known yet might be needed for parsing variable/member/parameter/return types.
	shared_ptr<CppUnresolvedFunction> func = make_shared<CppUnresolvedFunction>();
	func->sourceCode = make_shared<CppSourceCodeBuffer>(functionDeclRange);
	func->namespacePrefix = namespacePrefix;
	func->enclosingClassIfExists = enclosingClassIfExists;
	func->comment = parseCommentLinesAboveDeclaration(functionDeclStartOffset);
	parsedFile->unresolvedFunctions.push_back(func);
}

void CppFileParser::scanForExportDeclarationsSub(CppElementIterator &elementIter, const string &namespacePrefix, CppClass *enclosingClassIfExists, bool insideUnexportedClass) {
	// This variable indicates that the current statement is probably a class declaration without an export keyword,
	// which means that if a member function is found with an export keyword, a warning will be shown, because the user
	// probably forgot to add the export keyword to the class.
	bool unexportedClassDeclarationWithinCurrentStatement = false;

	bool previousKeywordWasUsing = false;

	while (elementIter.isValid()) {
		CppElementIndex startIndex = elementIter.element.startIndex;

		bool currentKeywordIsUsing = false;

		if (elementIter.element.type == CppElementType::IDENTIFIER) {
			bool isExportKeyword = elementIter.element.text == exportKeyword;
			bool isExportKeywordWithParameters = elementIter.element.text == exportKeywordWithParameters;
			if (isExportKeyword || isExportKeywordWithParameters) {
				LOG_DEBUG(sb() << "Found export keyword at: " << elementIter.getSourceLocation().toString());
				if (!file->isHeaderFile) elementIter.throwParseException(sb() << config->programName << " supports exporting declarations only from header files, not from source files.");
				elementIter.moveToNext();

				shared_ptr<CppExportParameters> exportParameters = make_shared<CppExportParameters>();
				if (isExportKeywordWithParameters) {
					exportParameters = parseExportParameters(elementIter);
				}
				else {
					if (elementIter.isValid() && elementIter.element.ch == '(') elementIter.throwParseException(sb() << "Export keyword " << config->exportKeyword << " does not support parameters (use " << config->exportKeywordWithParameters << " instead if you intended to give parameters)");
				}

				if (!elementIter.isValid()) elementIter.throwParseException("Expected function/class declaration to export");
				parseExportDeclaration(elementIter, namespacePrefix, enclosingClassIfExists, exportParameters, insideUnexportedClass);
			}
			else if (elementIter.element.text == "class" || elementIter.element.text == "struct") {
				unexportedClassDeclarationWithinCurrentStatement = true;
			}
			else if (elementIter.element.ch == ';') {
				unexportedClassDeclarationWithinCurrentStatement = false;
			}
			else if (elementIter.element.text == "using") {
				currentKeywordIsUsing = true;
			}
			else if (elementIter.element.text == "namespace") {
				// Check if this is a namespace declaration (and not a "using namespace" declaration).
				if (!previousKeywordWasUsing) {
					elementIter.moveToNext();
					if (!elementIter.isValid()) elementIter.throwParseException("Expected namespace name");
					if (elementIter.element.type == CppElementType::IDENTIFIER) { // If not a text element, could be an unnamed namespace, which is ignored.
						string name = elementIter.element.text;
						elementIter.moveToNext();

						if (!elementIter.isValid()) elementIter.throwParseException("Expected namespace block");
						if (elementIter.element.ch == '{') {
							int oldBlockDepthLimit = elementIter.enterBlockAndGetOldLimit();
							scanForExportDeclarationsSub(elementIter, namespacePrefix + name + "::", enclosingClassIfExists, insideUnexportedClass);
							elementIter.exitBlock(oldBlockDepthLimit, '}', true);
						}
					}
				}
			}
		}
		else if (elementIter.element.ch == '{') {
			int oldBlockDepthLimit = elementIter.enterBlockAndGetOldLimit();
			scanForExportDeclarationsSub(elementIter, namespacePrefix, enclosingClassIfExists, insideUnexportedClass | unexportedClassDeclarationWithinCurrentStatement);
			elementIter.exitBlock(oldBlockDepthLimit, '}', true);
			unexportedClassDeclarationWithinCurrentStatement = false;
		}

		previousKeywordWasUsing = currentKeywordIsUsing;

		if (elementIter.isValid() && elementIter.element.startIndex.pos == startIndex.pos) {
			elementIter.moveToNextAndSkipSubElements();
		}
	}
}

void CppFileParser::scanForExportDeclarations(const CppSourceCodeView &elements, const string &namespacePrefix, CppClass *enclosingClassIfExists, bool insideUnexportedClass) {
	CppElementIterator elementIter(elements, commentMap.get());
	scanForExportDeclarationsSub(elementIter, namespacePrefix, enclosingClassIfExists, insideUnexportedClass);
}

bool CppFileParser::quickScanForPossibleExportKeyword() {
	// First check that the parameterized export keyword has the regular export keyword as a prefix, because otherwise would have to search with both.
	string keyword = config->exportKeyword;
	if (keyword.empty()) EXC("Export keyword cannot be empty, check configuration.");

	if (!boost::starts_with(config->exportKeywordWithParameters, keyword)) {
		EXC(sb() << "Parameterized export keyword " << config->exportKeywordWithParameters << " should have the regular export keyword " << config->exportKeyword << " as a prefix");
	}

	char firstChar = keyword.at(0);
	const char *data = (const char *)content.getBuffer();
	int64_t searchLength = content.getPosition() - (int64_t)keyword.size() + 1;
	for (int64_t i = 0; i < searchLength; i++) {
		if (data[i] == firstChar) {
			if (memcmp(data + i, keyword.data(), keyword.size()) == 0) {
				return true;
			}
		}
	}
	return false;
}

void CppFileParser::parseFileSub() {
	LOG_DEBUG(sb() << "Parsing source file: " << file->getPath());
	StopWatch stopWatch = app->createStopWatchForPerformanceMeasurement();

	content.setPosition(0);
	loadTextFileAsUtf8(&content, Path(file->getPath()));

	stopWatch.mark("parseFile / load text file");

	bool mayHaveExportKeywords = quickScanForPossibleExportKeyword();

	stopWatch.mark("parseFile / quick file check");

	if (mayHaveExportKeywords) {
		file->lineAndColumnNumberMap = make_shared<LineAndColumnNumberMap>();
		file->lineAndColumnNumberMap->build((const char *)content.getBuffer(), content.getPosition());
		stopWatch.mark("parseFile / line numbers and line-break counts");

		parsedFile->errorList.runWithExceptionCheck([&]() {
			CppSourceCodeView sourceCode(content.getBuffer(), content.getPosition(), 0, file);
			scanForExportDeclarations(sourceCode, "", nullptr, false);
		});
		stopWatch.mark("parseFile / parse");
	}
}

void CppFileParser::parseFile(CppParsedFile *parsedFile, Config *config) {
	CppFileParser p;
	p.config = config;
	p.exportKeyword = config->exportKeyword;
	p.exportKeywordWithParameters = config->exportKeywordWithParameters;
	p.file = parsedFile->file.get();
	p.commentMap = make_shared<CppCommentMap>();
	p.parsedFile = parsedFile;
	p.parseFileSub();
}

