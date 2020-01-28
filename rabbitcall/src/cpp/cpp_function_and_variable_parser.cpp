#include "pch.h"

CppFuncVarParser::CppFuncVarParser(Config *config, TypeMap *typeMap)
	: config(config), typeMap(typeMap) {
}

shared_ptr<CppFuncVar> CppFuncVarParser::parseLambdaFunctionType(CppElementIterator &elementIter) {
	if (!elementIter.isValid() || elementIter.element.ch != '<') elementIter.throwParseException("Expected '<' for function type");
	elementIter.moveToNext();

	// Parse return value type.
	shared_ptr<CppFuncVar> result = parseVariableDeclaration(elementIter, false, nullptr).decl;
	if (result->isLambdaFunction) throw ParseException(result->sourceLocation, "Lambda function as a return value of a lambda function is not supported");
	result->isLambdaFunction = true;

	// Parse parameter types.
	if (!elementIter.isValid() || elementIter.element.ch != '(') elementIter.throwParseException(sb() << "Expected function parameter list");
	int oldBlockDepthLimit = elementIter.enterBlockAndGetOldLimit();
	while (elementIter.isValid()) {
		CppElementIterator::InfiniteLoopGuard loopGuard(&elementIter);

		ParseVariableResult variableResult = parseVariableDeclaration(elementIter, true, nullptr);
		if (variableResult.decl->isLambdaFunction) throw ParseException(variableResult.decl->sourceLocation, "Lambda function as a parameter of a lambda function is not supported");
		if (variableResult.decl->declarationName.empty()) throw ParseException(variableResult.decl->sourceLocation, "Lambda function parameters must have a name");
		if (variableResult.decl->arraySize > 0) throw ParseException(variableResult.decl->sourceLocation, "Arrays not supported for lambda function parameters");
		result->addFunctionParameterType(variableResult.decl);

		loopGuard.checkProgress();
	}
	elementIter.exitBlock(oldBlockDepthLimit, ')', false);

	return result;
}

TypeMapping * CppFuncVarParser::tryParsePlainVariableType(CppElementIterator &elementIter) {
	TypeMappingTrieNode *node = typeMap->getTypeMappingRootNodeAndCheckAllTypesAvailable();
	TypeMapping *bestType = nullptr;
	CppElementIndex bestTypeEndIndexOrRangeStartIndex = elementIter.element.startIndex;

	// Iterate through name parts (e.g. in "signed long ns::long", there are 3 parts: "signed", "long", "ns::long").
	while (elementIter.isValid()) {
		CppElementIterator::InfiniteLoopGuard loopGuard(&elementIter);

		CppElementIndex partStartIndex = elementIter.element.startIndex;

		// Parse the next space-separated part (that may have a partial or full namespace prefix).
		string name = CppItemParseUtil::tryParsePossiblyQualifiedName(elementIter);
		if (name.empty()) break;

		// Try to append the current identifier to the name and move to the corresponding node in the trie structure.
		// If a node is not found, then it's not possible that the name would become valid after the current identifier
		// and possibly more identifiers are appended.
		node = node->getSubNodeByNameIfExists(name);
		if (!node) break;

		if (node->isTypeAmbiguous()) elementIter.getSourceCode()->throwParseExceptionByRelativePos(partStartIndex.pos, sb() << "Ambiguous type: " << name);

		// Check if the current name combination is valid and if it is, save it as the best candidate so far.
		// The longest valid name will become the result (e.g. "signed long long" instead of "signed long").
		// Note that it's possible that the current name is not valid now, but becomes valid after more parts are appended.
		TypeMapping *type = node->getTypeIfUnique();
		if (type) {
			bestType = type;
			bestTypeEndIndexOrRangeStartIndex = elementIter.element.startIndex;
		}

		loopGuard.checkProgress();
	}

	elementIter.moveTo(bestTypeEndIndexOrRangeStartIndex);
	return bestType; // Null if not found.
}

CppFuncVarParser::ParseVariableResult CppFuncVarParser::parseVariableDeclaration(CppElementIterator &elementIter, bool shouldParseVariableName, shared_ptr<CppFuncVar> *baseTypePtrIfVariableList) {
	shared_ptr<CppFuncVar> variableDecl;
	CppElementIndex startIndex = elementIter.element.startIndex;
	int64_t startFileOffset = elementIter.getElementStartFileOffset();
	bool isStatic = false;
	bool doesListContinueAfterThisVariable = false;
	int64_t pointerDepth = 0;
	int64_t referenceDepth = 0;
	int64_t explicitAlignment = 0;
	CppElementIndex variableNameElementIndex;
	CppElementIndex arraySizeElementIndex;

	// If this is variable in a comma-separated list of variables, use the previously parsed type from the first variable.
	bool isFirstVariableInList = true;
	if (baseTypePtrIfVariableList && *baseTypePtrIfVariableList) {
		variableDecl = make_shared<CppFuncVar>();
		*variableDecl = **baseTypePtrIfVariableList; // Take a copy of the base type so that the base type can be used for later variables.
		isFirstVariableInList = false;
	}

	while (elementIter.isValid()) {
		CppElementIterator::InfiniteLoopGuard loopGuard(&elementIter);

		if (elementIter.element.ch == ',') {
			elementIter.moveToNext();
			doesListContinueAfterThisVariable = true;
			break;
		}

		if (elementIter.element.ch == '=') {
			elementIter.moveToNext();
			// If there is an initializer to assign a default value to the variable, skip it.
			while (elementIter.isValid()) {
				if (elementIter.element.ch == ',') {
					elementIter.moveToNext();
					doesListContinueAfterThisVariable = true;
					break;
				}
				if (elementIter.element.ch == ';') {
					elementIter.moveToNext();
					break;
				}
				elementIter.moveToNextAndSkipSubElements();
			}
			break;
		}

		if (elementIter.element.ch == ';') {
			elementIter.moveToNext();
			break;
		}

		// The return type of a lambda function ends at a parenthesis.
		if (elementIter.element.ch == '(') {
			break;
		}

		if (elementIter.element.text == "alignas") {
			elementIter.moveToNext();
			explicitAlignment = max(explicitAlignment, CppItemParseUtil::parseAlignasParameter(elementIter));
		}
		else {
			bool wasCurrentElementVariableType = false;
			if (!variableDecl) {
				// Use the first recognizable variable type name.
				TypeMapping *type = tryParsePlainVariableType(elementIter);
				if (type) {
					wasCurrentElementVariableType = true;
					if (type->typeNames.cppType == "std::function") {
						variableDecl = parseLambdaFunctionType(elementIter);
					}
					else {
						variableDecl = make_shared<CppFuncVar>();
						variableDecl->type = type;
					}
				}
			}

			if (!wasCurrentElementVariableType) {
				bool wasCurrentElementKnownIdentifier = false;

				// The current element was not the beginning of a variable type => check if it is a known keyword.
				if (isFirstVariableInList) { // Some keywords can only appear with the first variable.
					if (elementIter.element.type == CppElementType::IDENTIFIER) {
						if (elementIter.element.text == "static") {
							isStatic = true; // Static can appear before or after the variable type, but only with the first variable in a list.
							wasCurrentElementKnownIdentifier = true;
						}
					}
				}

				if (variableDecl) { // Check if variable type already found.
					// Check for pointer '*' and reference '&' chars if the variable type has already been found.
					// There can be "const" keywords between the '*' and '&' chars.
					if (elementIter.element.ch == '&') {
						if (pointerDepth > 0) elementIter.throwParseException("Cannot have a pointer to a reference.");
						referenceDepth++;
					}
					else if (elementIter.element.ch == '*') {
						pointerDepth++;
					}

					if (!wasCurrentElementKnownIdentifier && elementIter.element.type == CppElementType::IDENTIFIER) {
						// Found a new candidate for variable name.
						variableNameElementIndex = elementIter.element.startIndex;
						arraySizeElementIndex = CppElementIndex(); // If array brackets have been found before variable name, ignore (could be attributes).
					}
					else if (elementIter.element.ch == '[') {
						arraySizeElementIndex = elementIter.element.startIndex;
					}
				}

				elementIter.moveToNextAndSkipSubElements();
			}
		}

		loopGuard.checkProgress();
	}

	if (!variableDecl) {
		elementIter.getSourceCode()->throwParseExceptionByRelativePos(startIndex.pos, sb() << "Could not parse type (maybe missing type mapping in configuration, or missing " << config->exportKeyword << " keyword at type declaration?)");
	}

	variableDecl->isStatic = isStatic;
	variableDecl->sourceLocation = elementIter.getFile()->getSourceLocationByFileOffset(startFileOffset);

	if (baseTypePtrIfVariableList && doesListContinueAfterThisVariable) {
		// Store the variable type from the first variable in a list for the subsequent variables that share the same type.
		*baseTypePtrIfVariableList = make_shared<CppFuncVar>();
		**baseTypePtrIfVariableList = *variableDecl;
	}

	if (shouldParseVariableName && variableNameElementIndex.isValid()) {
		CppElementIterator variableNameElement(*elementIter.getSourceCode(), variableNameElementIndex, CPP_INVALID_BLOCK_DEPTH, nullptr);
		variableDecl->declarationName = variableNameElement.element.text;
	}

	int64_t arraySize = 0;
	if (arraySizeElementIndex.isValid()) {
		CppSourceCodeView arraySizeElements = CppElementIterator(*elementIter.getSourceCode(), arraySizeElementIndex, CPP_INVALID_BLOCK_DEPTH, nullptr).getSubElements();
		string arraySizeText = arraySizeElements.getSimplifiedCode();
		if (!tryParseInt64(arraySizeText, &arraySize) || arraySize < 0 || arraySize > 0x7FFFFFFF) {
			arraySizeElements.throwParseExceptionAtStart(sb() << "Invalid array size: " << arraySizeText);
		}
		LOG_DEBUG(sb() << "Parsed array size: " << arraySize << " from '" << arraySizeText << "'");
	}
	variableDecl->arraySize = (int)arraySize;

	if (pointerDepth < 0 || pointerDepth >= 256) elementIter.getSourceCode()->throwParseExceptionByRelativePos(startIndex.pos, sb() << "Invalid pointer depth: " << pointerDepth);
	if (referenceDepth >= 2) elementIter.getSourceCode()->throwParseExceptionByRelativePos(startIndex.pos, "More than one level of reference ('&') not supported, including rvalue references");
	variableDecl->pointerDepth = (int)pointerDepth;
	variableDecl->referenceDepth = (int)referenceDepth;
	variableDecl->explicitAlignment = (int)explicitAlignment;

	if (variableDecl->isLambdaFunction && variableDecl->pointerDepth > 0) elementIter.getSourceCode()->throwParseExceptionByRelativePos(startIndex.pos, "Pointer to std::function object not supported (use \"const std::function<void()> &\")");

	ParseVariableResult result;
	result.decl = variableDecl;
	result.doesListContinue = doesListContinueAfterThisVariable;
	return result;
}

CppFuncVarParser::FunctionIdentifyResult CppFuncVarParser::tryIdentifyFunction(const CppSourceCodeView &elements) {
	FunctionIdentifyResult result;
	if (elements.isEmpty()) {
		return result;
	}

	// Find the last parentheses-enclosed part before an equal sign, and if the equal sign is not a part of an operator, assume it is a function parameter list
	// (earlier parentheses may be used in e.g. __declspec(thread)). Parentheses appearing after an equal sign could be a part of a variable initializer in a struct and not a function definition.
	CppElementIterator firstElement(elements, nullptr);
	CppElementIndex startIndex = firstElement.element.startIndex;
	CppElementIndex paramElementIndex;
	{
		CppElementIterator elementIter = firstElement;
		bool nextEqualSignCouldBePartOfOperatorOverload = false;
		while (elementIter.isValid()) {
			if (elementIter.element.ch == '(') {
				// This is the last element so far that looks like a parameter list.
				paramElementIndex = elementIter.element.startIndex;
				nextEqualSignCouldBePartOfOperatorOverload = false;
			}
			else if (elementIter.element.text == "alignas") {
				// Avoid misidentifying the parentheses after an "alignas" keyword as a function parameter list.
				elementIter.moveToNext();
				// The parentheses will be skipped below.
			}
			else if (elementIter.element.text == "operator") {
				nextEqualSignCouldBePartOfOperatorOverload = true;
			}
			else if (elementIter.element.ch == '=') {
				if (!nextEqualSignCouldBePartOfOperatorOverload) { // If this looks like an operator overload, assume the equal sign is part of it and not a variable initializer.
					elementIter.moveToNext();
					break;
				}
			}

			elementIter.moveToNextAndSkipSubElements();
		}
	}
	if (!paramElementIndex.isValid()) {
		return result;
	}

	result.isFunction = true;
	result.startIndex = startIndex;
	result.paramElementIndex = paramElementIndex;
	return result;
}

shared_ptr<CppFuncVar> CppFuncVarParser::tryParseFunctionDeclaration(const CppSourceCodeView &elements, const string &namespacePrefix) {
	FunctionIdentifyResult identifyResult = tryIdentifyFunction(elements);
	if (!identifyResult.isFunction) {
		return nullptr;
	}
	
	CppElementIterator paramIter(elements, identifyResult.paramElementIndex, CPP_INVALID_BLOCK_DEPTH, nullptr);

	CppSourceCodeView functionNameAndReturnTypeElements = elements.getSubRange(identifyResult.startIndex, identifyResult.paramElementIndex);
	if (functionNameAndReturnTypeElements.isEmpty()) elements.throwParseExceptionAtStart("Expected function return type and name before parameter list");

	// Parse the return type and name as a "variable".
	CppElementIterator nameAndReturnTypeIter = CppElementIterator(functionNameAndReturnTypeElements, nullptr);
	shared_ptr<CppFuncVar> functionDecl = parseVariableDeclaration(nameAndReturnTypeIter, true, nullptr).decl;
	functionDecl->namespacePrefixIfGlobal = namespacePrefix;
	if (functionDecl->arraySize > 0) elements.throwParseExceptionAtStart("Arrays not supported for function return values");
	if (functionDecl->declarationName.empty()) paramIter.throwParseException("Expected function name before parameter list");

	// Parse function parameters.
	int oldBlockDepthLimit = paramIter.enterBlockAndGetOldLimit();
	while (paramIter.isValid()) {
		CppElementIterator::InfiniteLoopGuard loopGuard(&paramIter);

		ParseVariableResult variableResult = parseVariableDeclaration(paramIter, true, nullptr); // Parse as a list of variables, but don't use the first variable's type for the other variables.
		functionDecl->addFunctionParameterType(variableResult.decl);
		if (functionDecl->arraySize > 0) paramIter.throwParseException("Arrays not supported for function parameters");

		if (!variableResult.doesListContinue) {
			break;
		}

		loopGuard.checkProgress();
	}
	paramIter.exitBlock(oldBlockDepthLimit, ')', false);

	// Parse keywords after the parameter list (e.g. noexcept, const).
	while (paramIter.isValid()) {
		if (paramIter.element.ch == '{' || paramIter.element.ch == ';' || paramIter.element.ch == '-') { // The '-' might be the start of a trailing return type.
			break;
		}

		if (paramIter.element.text == "noexcept") {
			functionDecl->isNoexcept = true;
		}

		paramIter.moveToNextAndSkipSubElements();
	}

	return functionDecl;
}

