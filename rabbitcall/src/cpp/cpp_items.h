#pragma once

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// The order is from least strict to most strict (can be used for comparison as ints).
enum class CppAccessModifier {
	UNKNOWN,
	PUBLIC,
	PROTECTED,
	PRIVATE
};

// Used when e.g. converting superclass public functions to private when the superclass is inherited as private.
CppAccessModifier getStrictenedAccessModifier(CppAccessModifier original, CppAccessModifier strictening);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Parameters that affect how a class/function is exported to other languages from C++.
struct CppExportParameters {

	struct HlslParameters {
		bool exportHlsl = false;
		bool isCBuffer = false;
		string fieldPrefix;
		string registerName;
	};

	struct GlslParameters {
		bool exportGlsl = false;
		string storage;
		string instanceName;
		string fieldPrefix;
		string binding;
	};

	HlslParameters hlslParameters;
	GlslParameters glslParameters;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct CppComment {
	vector<string> lines;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// C++ function or variable declaration.
class CppFuncVar {
public:
	string declarationName; // Empty means this object represents only a type without a name.
	string namespacePrefixIfGlobal;
	TypeMapping *type = nullptr; // For functions, this is the return value type.
	int pointerDepth = 0; // How many pointer asterisks there are after the C++ type name.
	int referenceDepth = 0;
	int arraySize = 0; // Zero means not an array.
	int explicitAlignment = 0;
	bool isStatic = false;
	bool isNoexcept = false;
	bool isLambdaFunction = false;
	CppAccessModifier accessModifier = CppAccessModifier::PUBLIC;
	vector<shared_ptr<CppFuncVar>> functionParameters;
	unordered_map<string, string> functionEntryPointNameByContext;
	shared_ptr<CppComment> comment;
	SourceLocation sourceLocation;

	CppFuncVar() {
	}

	string getNamespaceName() const;
	void addFunctionParameterType(shared_ptr<CppFuncVar> v);
	bool isVoid() const;
	CppFuncVar getFunctionReturnType() const;
	CppFuncVar getFunctionReturnTypeAndName() const;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// The offsets, sizes and types of data structure fields.
class CppStructLayout {
public:

	struct Field {
		int64_t offset = 0;
		int64_t elementSize = 0;
		int64_t alignment = 0;
		int64_t pointerDepth = 0;
		int64_t arraySize = 0;

		string name;
		TypeNamesByLanguage typeNames;

		CppAccessModifier accessModifier = CppAccessModifier::PUBLIC;

		shared_ptr<CppComment> comment;
		SourceLocation sourceLocation;
	};

	size_t size = 0;
	size_t alignment = 0;
	vector<Field> fields;

	CppStructLayout() = default;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct CppEnumField {
	string name;
	string value;
	shared_ptr<CppComment> comment;
	SourceLocation sourceLocation;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

enum class CppClassDeclarationType {
	UNKNOWN,
	CLASS,
	STRUCT,
	ENUM
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// The name and access type of a base class (when it has been parsed from source code but not yet linked to the class definition).
struct CppSuperclassRef {
	string partialName;
	string fullyQualifiedName; // Available only after all input files have been read and exported classes identified.
	CppAccessModifier accessModifier;
	SourceLocation sourceLocation;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct CppUnresolvedMember {
	// Contains partially-parsed source code for each member declaration that can be fully resolved only after all input files have been read and types are known.
	shared_ptr<CppSourceCodeBuffer> sourceCode;
	shared_ptr<CppComment> comment;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// C++ class/struct/enum.
class CppClass {
public:
	CppClassDeclarationType classType = CppClassDeclarationType::UNKNOWN;
	TypeNamesByLanguage typeNames;
	int64_t explicitAlignment = 0;
	bool isClassEnum = false; // "enum class Xyz" instead of just "enum Xyz".
	bool isPassByValue = false;
	shared_ptr<TypeMapping> typeMapping;
	shared_ptr<CppStructLayout> layout;
	shared_ptr<CppExportParameters> exportParameters;
	shared_ptr<CppComment> comment;
	SourceLocation sourceLocation;

	vector<CppUnresolvedMember> unresolvedMembers;
	vector<shared_ptr<CppFuncVar>> functions;
	vector<shared_ptr<CppFuncVar>> fields;
	vector<CppEnumField> enumFields;
	vector<CppSuperclassRef> superclasses;

	unordered_set<CppClass *> dependeeClasses;
	unordered_set<CppClass *> dependentClasses;
	unordered_set<CppClass *> unprocessedDependeeClasses;

	CppClass() = default;

	CppStructLayout * getLayoutOrThrow() const;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// A function declaration that has been encountered in source code but has not yet been parsed, because not all types in the project are known yet
// (which may affect parsing because heuristics are used for identifying types in the declaration).
struct CppUnresolvedFunction {
	shared_ptr<CppSourceCodeBuffer> sourceCode;
	string namespacePrefix;
	CppClass *enclosingClassIfExists = nullptr;
	shared_ptr<CppComment> comment;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct CppStatistics {
	int64_t numSourceFiles = 0;
	int64_t numClasses = 0;
	int64_t numFunctions = 0;
	int64_t numSourceBytes = 0;

	void add(CppStatistics *stats) {
		numSourceFiles += stats->numSourceFiles;
		numClasses += stats->numClasses;
		numFunctions += stats->numFunctions;
		numSourceBytes += stats->numSourceBytes;
	}
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace CppItemParseUtil {

	CppAccessModifier parseAccessModifier(const string &text);

	// Tries to read the given characters, which may span multiple elements but must not have spaces in the middle.
	bool tryParseSpecialChars(CppElementIterator &elementIter, const char *s);

	// Parses e.g. "string" or "std::string".
	string tryParsePossiblyQualifiedName(CppElementIterator &elementIter);

	unordered_map<string, CppSourceCodeView> parseCommaSeparatedParameterMap(const CppSourceCodeView &elements);

	bool isSameTypeSignature(CppFuncVar *f1, CppFuncVar *f2);

	bool tryParseInt64InParenthesis(CppElementIterator &elementIter, char parenthesisType, int64_t *result);
	int64_t parseAlignasParameter(CppElementIterator &elementIter);
};

