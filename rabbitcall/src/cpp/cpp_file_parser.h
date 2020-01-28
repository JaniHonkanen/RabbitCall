#pragma once

class CppParsedFile {
public:
	shared_ptr<CppFile> file;
	vector<shared_ptr<CppUnresolvedFunction>> unresolvedFunctions;
	vector<shared_ptr<CppClass>> classes;
	ErrorList errorList;

	explicit CppParsedFile(const shared_ptr<CppFile> &file);

	bool hasExportedElements() {
		return !unresolvedFunctions.empty() || !classes.empty();
	}
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Parses a C++ source/header file and scans for functions/classes etc. to export to C#.
// Different files may be parsed concurrently by different instances of this class.
class CppFileParser {
	Config *config = nullptr;
	CppFile *file = nullptr;
	shared_ptr<CppCommentMap> commentMap;
	CppParsedFile *parsedFile = nullptr;
	ByteBuffer content;
	string exportKeyword;
	string exportKeywordWithParameters;

	// Parses parameters in an export keyword (FXP).
	shared_ptr<CppExportParameters> parseExportParameters(CppElementIterator &elementIter);

	string getLineText(int64_t lineNumber);

	bool isLineAcceptedBetweenCommentAndDeclaration(int64_t lineNumber);
	shared_ptr<CppComment> parseCommentLinesAboveDeclaration(int64_t declarationOffset);

	void parseSuperclassListElement(CppElementIterator &elementIter, CppAccessModifier defaultAccessModifier, const function<void(const CppSuperclassRef &)> &resultCallback);
	void parseSuperclassList(CppElementIterator &elementIter, CppAccessModifier defaultAccessModifier, const function<void(const CppSuperclassRef &)> &resultCallback);

	// Tries to identify a function/variable declaration without full information about available types. Needed when a type used by this function/variable
	// is declared in another source file that has not been processed yet (because the parser does not process #include files first in the same way as a normal C++ compiler).
	CppSourceCodeView extractFunctionOrVariableDeclaration(CppElementIterator &elementIter);

	bool tryParseClass(CppElementIterator &elementIter, const string &namespacePrefix, CppClass *enclosingClassIfExists, shared_ptr<CppExportParameters> exportParameters);
	void parseStructBody(CppElementIterator &elementIter, CppClass *classDecl);
	void parseEnumBody(CppElementIterator &elementIter, CppClass *classDecl);

	// Tries to parse a class/function export declaration after an export keyword (FXP).
	void parseExportDeclaration(CppElementIterator &elementIter, const string &namespacePrefix, CppClass *enclosingClassIfExists, shared_ptr<CppExportParameters> exportParameters, bool insideUnexportedClass);

	// Scans for export keywords (FXP) in source code and identifies classes/functions to export.
	void scanForExportDeclarationsSub(CppElementIterator &elementIter, const string &namespacePrefix, CppClass *enclosingClassIfExists, bool insideUnexportedClass);
	void scanForExportDeclarations(const CppSourceCodeView &elements, const string &namespacePrefix, CppClass *enclosingClassIfExists, bool insideUnexportedClass);

	// Quickly scans if the file could even possibly contain export keywords before parsing the file fully.
	bool quickScanForPossibleExportKeyword();

	void parseFileSub();

public:
	static void parseFile(CppParsedFile *parsedFile, Config *config);
};

