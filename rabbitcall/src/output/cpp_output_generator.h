#pragma once

class CppOutputGenerator : public OutputFileGenerator {
	
	TypeMap *typeMap;
	shared_ptr<CppOutputFile> generatedHeaderFile;

	// Outputs a struct that holds a GC reference to the C# callback method.
	void outputCallbackWrapper(CppFuncVar *callbackParam, CppFuncVar *enclosingFunction, CppClass *enclosingClassIfNotGlobal, StringBuilder &output, string *wrapperClassNameOut);

	// Outputs a C++ function wrapper than can be called from C#.
	void outputFunction(CppFuncVar *func, CppClass *enclosingClassIfNotGlobal, StringBuilder &output);

	// Outputs a C++ class wrapper that can be used from C#.
	void outputClass(CppClass *clazz, StringBuilder &output);

	void outputEnum(CppClass *clazz, StringBuilder &output);

public:
	CppOutputGenerator() = default;
	DISABLE_COPY_AND_MOVE(CppOutputGenerator);
	virtual ~CppOutputGenerator() = default;

	string getFileType() override { return "cpp"; }
	void generateOutput(StringBuilder &output) override;
};
