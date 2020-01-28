#pragma once

// Handles qualified names depending on language, e.g. "std::string" or "std.string".
class NamespaceParser {

	string separator;

public:
	explicit NamespaceParser(const string &separator) : separator(separator) {
	}

	// E.g. for input ns1::ns2::mytype the callback will be called with ns1::ns2::mytype, ns2::mytype, mytype.
	void forEachPartialNamespaceSuffix(const string &name, const function<void(const string &)> &c);

	// Returns the parts in a qualified namespace name, or an empty array if an empty string is given.
	vector<string> parseNamespaceParts(const string &ns);

	void separateNamespaceAndName(string &namespaceNameOut, string &unqualifiedNameOut, const string &qualifiedName);
	string getNamespaceNameFromQualifiedName(const string &qualifiedName);
	string getUnqualifiedNameFromQualifiedName(const string &qualifiedName);
	string convertToSeparator(const string &qualifiedName, const string &newSeparator);
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

enum class CppCharType {
	UNKNOWN = 0,
	HORIZONTAL_SPACE,
	CR,
	LF,
	SINGLE_QUOTE,
	DOUBLE_QUOTE,
	SLASH,
	BACKSLASH,
	NUMBER_SIGN,
	BLOCK_START,
	BLOCK_END,

	// Only one flag may be set at the same time, but a flag may be set together with lower bits representing a more detailed char type.
	MAX_TYPES = 0x20, // How many types can be represented in the lower bits below the flags.
	FLAG_OPERATOR_CHAR = 0x20,
	FLAG_DIGIT = 0x40,
	FLAG_LETTER_OR_UNDERSCORE = 0x80,
};

class CppParseUtil {

	uint8_t charTypes[256] = {};
	
public:
	CppParseUtil();

	NamespaceParser cppNamespaceParser = NamespaceParser("::");
	NamespaceParser csNamespaceParser = NamespaceParser(".");

	uint8_t getCharType(char ch) { return charTypes[(int)ch]; }
	bool isLetterOrUnderscore(char ch) { return getCharType(ch) & (uint8_t)CppCharType::FLAG_LETTER_OR_UNDERSCORE; }
	bool isDigit(char ch) { return getCharType(ch) & (uint8_t)CppCharType::FLAG_DIGIT; }
	bool isLetterOrUnderscoreOrDigit(char ch) { return getCharType(ch) & ((uint8_t)CppCharType::FLAG_LETTER_OR_UNDERSCORE | (uint8_t)CppCharType::FLAG_DIGIT); }
	bool isOperatorChar(char ch) { return getCharType(ch) & (uint8_t)CppCharType::FLAG_OPERATOR_CHAR; }
	bool isIdentifier(const string &s);
	bool isHorizontalSpace(char ch) { return getCharType(ch) == (uint8_t)CppCharType::HORIZONTAL_SPACE; }
	bool isWhiteSpace(char ch) { return isHorizontalSpace(ch) || ch == '\r' || ch == '\n'; }

	bool containsWhiteSpaceOnly(const char *data, int64_t length);
};
extern unique_ptr<CppParseUtil> cppParseUtil;


