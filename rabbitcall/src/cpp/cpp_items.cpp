#include "pch.h"

CppAccessModifier getStrictenedAccessModifier(CppAccessModifier original, CppAccessModifier strictening) {
	return (strictening == CppAccessModifier::UNKNOWN) ? original : (CppAccessModifier)(max((int)original, (int)strictening));
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

string CppFuncVar::getNamespaceName() const {
	string ns = namespacePrefixIfGlobal;
	return boost::ends_with(ns, "::") ? ns.substr(0, ns.size() - 2) : ns;
}

void CppFuncVar::addFunctionParameterType(shared_ptr<CppFuncVar> v) {
	functionParameters.push_back(v);
}

bool CppFuncVar::isVoid() const {
	return type && type->typeNames.cppType == "void" && pointerDepth == 0;
}

CppFuncVar CppFuncVar::getFunctionReturnType() const {
	CppFuncVar v;
	v.sourceLocation = sourceLocation;
	v.type = type;
	v.pointerDepth = pointerDepth;
	return v;
}

CppFuncVar CppFuncVar::getFunctionReturnTypeAndName() const {
	CppFuncVar v = getFunctionReturnType();
	v.declarationName = declarationName;
	return v;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CppStructLayout * CppClass::getLayoutOrThrow() const {
	if (!layout) EXC(sb() << "Layout missing for class/struct: " << typeNames.cppType);
	return layout.get();
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CppAccessModifier CppItemParseUtil::parseAccessModifier(const string &text) {
	if (text == "public") return CppAccessModifier::PUBLIC;
	if (text == "protected") return CppAccessModifier::PROTECTED;
	if (text == "private") return CppAccessModifier::PRIVATE;
	return CppAccessModifier::UNKNOWN;
}

bool CppItemParseUtil::tryParseSpecialChars(CppElementIterator &elementIter, const char *s) {
	CppElementIterator startElement = elementIter;

	int64_t i = 0;
	while (true) {
		char ch = s[i];
		if (ch == '\0') return true;
		if (!elementIter.isValid()) break;
		if (elementIter.element.ch != ch) break;
		i++;
	}

	elementIter = startElement; // Rewind to start.
	return false;
}

string CppItemParseUtil::tryParsePossiblyQualifiedName(CppElementIterator &elementIter) {
	CppElementIterator startElement = elementIter;

	string result;
	while (elementIter.isValid()) {
		CppElementIterator::InfiniteLoopGuard loopGuard(&elementIter);

		if (elementIter.element.type != CppElementType::IDENTIFIER) {
			// A name part should come next (either at the beginning or following a double colon) but didn't => invalid name.
			elementIter = startElement;
			return "";
		}
		result += elementIter.element.text;
		elementIter.moveToNextAndSkipSubElements();

		if (elementIter.element.text == "operator") {
			{
				CppElementIterator nextElementIter = elementIter.getNextAndSkipSubElements();
				if (nextElementIter.isValid()) {
					string text = nextElementIter.element.text;
					if (text == "new" || text == "delete" || text == "co_await") { // Operator new/delete/co_await
						result += " ";
						result += text;
						elementIter = nextElementIter;
					}
				}
			}

			// Read operator characters (+, -, * etc.)
			while (elementIter.isValid()) {
				char ch = elementIter.element.ch;
				if (ch == '\0' || !cppParseUtil->isOperatorChar(ch)) {
					break;
				}
				result += ch;
				elementIter.moveToNextAndSkipSubElements();
			}
		}

		if (tryParseSpecialChars(elementIter, "::")) {
			result += "::";
		}
		else {
			break;
		}

		loopGuard.checkProgress();
	}
	return result;
}

unordered_map<string, CppSourceCodeView> CppItemParseUtil::parseCommaSeparatedParameterMap(const CppSourceCodeView &elements) {
	unordered_map<string, CppSourceCodeView> result;

	CppElementIterator elementIter(elements, nullptr);
	while (elementIter.isValid()) {
		CppElementIterator::InfiniteLoopGuard loopGuard(&elementIter);

		CppElementIndex paramStartIndex = elementIter.element.startIndex;
		CppSourceCodeView paramElements = elementIter.findElementAndGetSourceCodeBeforeIt(true, [&] { return elementIter.element.ch == ','; });
		CppElementIterator paramIter(paramElements, nullptr);
		if (!paramIter.isValid()) paramIter.throwParseException("Empty parameter");

		if (paramIter.element.type != CppElementType::IDENTIFIER) paramIter.throwParseException(sb() << "Expected parameter name");
		string name = paramIter.element.text;
		if (result.find(name) != result.end()) paramIter.throwParseException(sb() << "Multiple instances of the same parameter: " << name);

		if (!paramIter.moveToNextAndSkipSubElements()) {
			result[name] = elements.getSubRange(paramStartIndex, paramStartIndex); // Create an empty element range that refers to the correct source code location.
		}
		else {
			if (paramIter.element.ch != '(') paramIter.throwParseException(sb() << "Expected parameter value in parentheses: " << paramIter.element.getText());
			result[name] = paramIter.getSubElements();

			if (paramIter.moveToNextAndSkipSubElements()) {
				paramIter.throwParseException(sb() << "Extra parameter elements after value");
			}
		}

		loopGuard.checkProgress();
	}

	return result;
}

bool CppItemParseUtil::isSameTypeSignature(CppFuncVar *f1, CppFuncVar *f2) {
	CHECK_NOT_NULL(f1);
	CHECK_NOT_NULL(f2);
	if (f1->type != f2->type) return false;
	if (f1->pointerDepth != f2->pointerDepth) return false;

	size_t numParameters = f1->functionParameters.size();
	if (numParameters != f2->functionParameters.size()) return false;
	for (size_t i = 0; i < numParameters; i++) {
		if (!isSameTypeSignature(f1->functionParameters[i].get(), f2->functionParameters[i].get())) return false;
	}
	return true;
}

bool CppItemParseUtil::tryParseInt64InParenthesis(CppElementIterator &elementIter, char parenthesisType, int64_t *result) {
	if (!elementIter.isValid() || elementIter.element.ch != parenthesisType) return false;
	CppSourceCodeView subElements = elementIter.getSubElements();
	if (!tryParseInt64(subElements.getSimplifiedCode(), result)) return false;
	elementIter.moveToNextAndSkipSubElements();
	return true;
}

int64_t CppItemParseUtil::parseAlignasParameter(CppElementIterator &elementIter) {
	int64_t alignment;
	if (!tryParseInt64InParenthesis(elementIter, '(', &alignment)) elementIter.throwParseException("Expected alignment parameter (e.g. alignas(16)) as a constant inline integer (expressions or macros not supported)");
	if (alignment != 0 && !isPowerOfTwo(alignment)) EXC(sb() << "Alignment must be a power-of-two: " << alignment);
	return alignment;
}

