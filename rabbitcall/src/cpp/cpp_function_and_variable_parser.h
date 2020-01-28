#pragma once

// Parses C++ function and variable declarations.
class CppFuncVarParser {

	Config *config = nullptr;
	TypeMap *typeMap = nullptr;
	
public:
	struct ParseVariableResult {
		shared_ptr<CppFuncVar> decl;
		bool doesListContinue = false;
	};

	struct FunctionIdentifyResult {
		CppElementIndex startIndex;
		CppElementIndex paramElementIndex;
		bool isFunction = false;
	};
	
	explicit CppFuncVarParser(Config *config, TypeMap *typeMap);

	// Parses the type of an std::function, e.g. "<void(int, double)>".
	shared_ptr<CppFuncVar> parseLambdaFunctionType(CppElementIterator &elementIter);

	// Parses a variable type name without additional keywords, but including qualified and multipart names, e.g. "int", "unsigned int", "string", "std::string".
	TypeMapping * tryParsePlainVariableType(CppElementIterator &elementIter);

	// Parses a variable declaration, possibly in a list of variables, e.g. "int t", "static int t, **t2[10]", "const std::string &t".
	ParseVariableResult parseVariableDeclaration(CppElementIterator &elementIter, bool shouldParseVariableName, shared_ptr<CppFuncVar> *baseTypePtrIfVariableList);

	// Checks if the given source code range represents a function.
	FunctionIdentifyResult tryIdentifyFunction(const CppSourceCodeView &elements);

	shared_ptr<CppFuncVar> tryParseFunctionDeclaration(const CppSourceCodeView &elements, const string &namespacePrefix);
};