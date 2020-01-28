#include "pch.h"

unique_ptr<CppParseUtil> cppParseUtil;

void NamespaceParser::forEachPartialNamespaceSuffix(const string &name, const function<void(const string &)> &c) {
	string partialName = name;
	while (true) {
		c(partialName);
		size_t i = partialName.find(separator);
		if (i == string::npos) break;
		partialName = partialName.substr(i + separator.size());
	}
}

vector<string> NamespaceParser::parseNamespaceParts(const string &ns) {
	vector<string> result;
	string remaining = ns;
	while (!remaining.empty()) {
		size_t i = remaining.find(separator);
		if (i == string::npos) {
			result.push_back(remaining);
			break;
		}
		result.push_back(remaining.substr(0, i));
		remaining = remaining.substr(i + separator.size());
	}
	return result;
}

void NamespaceParser::separateNamespaceAndName(string &namespaceNameOut, string &unqualifiedNameOut, const string &qualifiedName) {
	size_t i = qualifiedName.rfind(separator);
	if (i == string::npos) {
		namespaceNameOut = "";
		unqualifiedNameOut = qualifiedName;
	}
	else {
		namespaceNameOut = qualifiedName.substr(0, i);
		unqualifiedNameOut = qualifiedName.substr(i + separator.size());
	}
}

string NamespaceParser::getNamespaceNameFromQualifiedName(const string &qualifiedName) {
	string namespaceName;
	string unqualifiedName;
	separateNamespaceAndName(namespaceName, unqualifiedName, qualifiedName);
	return namespaceName;
}

string NamespaceParser::getUnqualifiedNameFromQualifiedName(const string &qualifiedName) {
	string namespaceName;
	string unqualifiedName;
	separateNamespaceAndName(namespaceName, unqualifiedName, qualifiedName);
	return unqualifiedName;
}

string NamespaceParser::convertToSeparator(const string &qualifiedName, const string &newSeparator) {
	string temp = qualifiedName;
	boost::replace_all(temp, separator, newSeparator);
	return temp;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CppParseUtil::CppParseUtil() {
	memset(charTypes, (uint8_t)CppCharType::UNKNOWN, sizeof(charTypes));

	charTypes[(uint32_t)' '] = (uint8_t)CppCharType::HORIZONTAL_SPACE;
	charTypes[(uint32_t)'\t'] = (uint8_t)CppCharType::HORIZONTAL_SPACE;
	charTypes[(uint32_t)'_'] = (uint8_t)CppCharType::FLAG_LETTER_OR_UNDERSCORE;
	charTypes[(uint32_t)'\r'] = (uint8_t)CppCharType::CR;
	charTypes[(uint32_t)'\n'] = (uint8_t)CppCharType::LF;
	charTypes[(uint32_t)'\''] = (uint8_t)CppCharType::SINGLE_QUOTE;
	charTypes[(uint32_t)'\"'] = (uint8_t)CppCharType::DOUBLE_QUOTE;
	charTypes[(uint32_t)'/'] = (uint8_t)CppCharType::SLASH;
	charTypes[(uint32_t)'\\'] = (uint8_t)CppCharType::BACKSLASH;
	charTypes[(uint32_t)'#'] = (uint8_t)CppCharType::NUMBER_SIGN;
	charTypes[(uint32_t)'('] = (uint8_t)CppCharType::BLOCK_START;
	charTypes[(uint32_t)'['] = (uint8_t)CppCharType::BLOCK_START;
	charTypes[(uint32_t)'{'] = (uint8_t)CppCharType::BLOCK_START;
	charTypes[(uint32_t)')'] = (uint8_t)CppCharType::BLOCK_END;
	charTypes[(uint32_t)']'] = (uint8_t)CppCharType::BLOCK_END;
	charTypes[(uint32_t)'}'] = (uint8_t)CppCharType::BLOCK_END;

	for (int i = 0; i < 256; i++) {
		char ch = (char)i;

		if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z')) {
			charTypes[i] = (uint8_t)CppCharType::FLAG_LETTER_OR_UNDERSCORE;
		}

		if (ch >= '0' && ch <= '9') {
			charTypes[i] = (uint8_t)CppCharType::FLAG_DIGIT;
		}
	}

	string operatorChars = "+-*/-<>!~&|^%=";
	for (int i = 0; i < (int)operatorChars.size(); i++) {
		uint8_t index = (uint8_t)operatorChars[i];
		charTypes[index] |= (uint8_t)CppCharType::FLAG_OPERATOR_CHAR;
	}
}

bool CppParseUtil::isIdentifier(const string &s) {
	if (s.empty()) return false;
	if (!isLetterOrUnderscore(s[0])) return false;
	for (size_t i = 1; i < s.size(); i++) {
		char ch = s[i];
		if (!isLetterOrUnderscoreOrDigit(ch)) return false;
	}
	return true;
}

bool CppParseUtil::containsWhiteSpaceOnly(const char *data, int64_t length) {
	for (int64_t i = 0; i < length; i++) {
		if (!isWhiteSpace(data[i])) return false;
	}
	return true;
}

