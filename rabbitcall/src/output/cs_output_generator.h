#pragma once

class CsOutputGenerator : public OutputFileGenerator {

	class NamespaceWriter {
		vector<string> namespaceParts;

	public:
		explicit NamespaceWriter(const string &namespaceName);
		void begin(StringBuilder &output);
		void end(StringBuilder &output);
	};

	struct Callback {
		CppFuncVar *callback;
		string publicDelegateTypeName;
	};

	// Outputs a function wrapper that allows calling a C++ function from C#.
	void outputFunction(CppFuncVar *func, CppClass *enclosingClassIfNotGlobal, int functionIndex, StringBuilder &output, vector<Callback> &callbacksOut);

	// Outputs a C# wrapper that contains a pointer to a C++ object.
	void outputClass(CppClass *clazz, StringBuilder &output);

	// Outputs the internal handlers that receive callbacks from C++ and pass them to the application delegates.
	void outputCallbackInternals(const string &className, vector<Callback> &callbacks, StringBuilder &output);

public:
	CsOutputGenerator() = default;
	DISABLE_COPY_AND_MOVE(CsOutputGenerator);
	virtual ~CsOutputGenerator() = default;

	string getFileType() override { return "cs"; }
	void generateOutput(StringBuilder &output) override;
};

