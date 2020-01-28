#pragma once

// Short identifiers used in generated code to reduce file size.
#define OUTPUT_PARAM_NAME_PREFIX "a"
#define OUTPUT_CALLBACK_ID_PARAM_NAME_PREFIX "cb"
#define OUTPUT_CALLBACK "_rc_Cb"
#define OUTPUT_CALLBACK_HOLDER "_rc_CbH"
#define OUTPUT_FUNCTION_NAME_PREFIX "_rc_f"
#define OUTPUT_RETURN_VALUE_PTR "_rc_r"
#define OUTPUT_RETURN_VALUE_TEMP "_rc_rt"
#define OUTPUT_RETURN_VALUE_TEMP2 "_rc_rt2"
#define OUTPUT_EXCEPTION_PTR "_rc_e"
#define OUTPUT_EXCEPTION_VAR "_rc_ex"
#define OUTPUT_CHECK_EXCEPTION "_rc_ce"
#define OUTPUT_THIS_PTR "_rc_t"
#define OUTPUT_CHECK_INIT "_rc_ci"
#define OUTPUT_INTERNAL_UTIL_CLASS "_rc_Ciu"
#define OUTPUT_CPP_LIBRARY_FILE "_rc_cl"
#define OUTPUT_FUNC_MACRO_EXC "_RC_FUNC_EXC"
#define OUTPUT_FUNC_MACRO_NOEXC "_RC_FUNC_NOEXC"
#define OUTPUT_CALLBACK_WRAPPER "_RC_CALLBACK"
#define OUTPUT_ALLOCATE_MEMORY "_rc_alloc"
#define OUTPUT_DEALLOCATE_MEMORY "_rc_dealloc"
#define OUTPUT_ALLOCATE_TASKMEM "_rc_allocTaskMem"
#define OUTPUT_DEALLOCATE_TASKMEM "_rc_deallocTaskMem"
#define OUTPUT_PTR_AND_SIZE "_rc_PtrAndSize"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// How a C++/C# type should be formatted in a particular context when generating glue code.
enum class TypePresentationType {
	// "Public" means that the type should be formatted as it is visible to the application, while "transfer" types are those that are used internally by the invocation system when transferring parameters between C++ and C#.
	CPP_PUBLIC,
	CPP_TRANSFER_PARAMETER, // Parameter from C# to a C++ function.
	CPP_TRANSFER_RETURN_VALUE, // Return value from C++ function to C#.
	CPP_TRANSFER_CALLBACK_PARAMETER, // Parameter to callback (from C++ to C#).
	CPP_TRANSFER_CALLBACK_RETURN_VALUE, // Return value from callback (from C# to C++).
	CS_PUBLIC,
	CS_TRANSFER_PARAMETER, // Parameter from C# to a C++ function.
	CS_TRANSFER_RETURN_VALUE, // Return value from C++ function to C#.
	CS_TRANSFER_CALLBACK_PARAMETER, // Parameter to callback (from C++ to C#).
	CS_TRANSFER_CALLBACK_RETURN_VALUE // Return value from callback (from C# to C++).
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// A common base class for output generators for different programming languages.
class OutputFileGenerator {
protected:
	CppPartition *partition = nullptr;
	CppPartition *mainPartition = nullptr;
	CppProject *cppProject = nullptr;
	Config *config = nullptr;
	CppOutputFile *outputFile = nullptr;
	Path outputDir;
	string cppLibraryFile;

public:
	explicit OutputFileGenerator() = default;
	DISABLE_COPY_AND_MOVE(OutputFileGenerator);
	virtual ~OutputFileGenerator() = default;

	CppPartition * getPartition() { return partition; }
	CppProject * getCppProject() { return cppProject; }
	const string & getCppLibraryFile() { return cppLibraryFile; }
	CppPartition * getMainPartition() { return mainPartition; }
	bool isMainPartition() { return partition == mainPartition; }

	void init(CppPartition *partition, CppPartition *mainPartition);
	CppOutputFile * getOutputFile() { return outputFile; }

	string getRelativePathForIncludeDirective(const string &file);

	string getAutogeneratedFileComment();
	CppFuncVar getFunctionReturnValuePtrType(const CppFuncVar *func);

	// Formats a type for either C++ or C# glue code.
	string formatDeclaration(const CppFuncVar &declaration, const string &displayName, TypePresentationType presentation);
	string formatDeclaration(const CppFuncVar &declaration, TypePresentationType presentation);

	bool outputComment(CppComment *comment, StringBuilder &output);

	bool updateGeneratedUtf8FileIfModified(const Path &path, const void *data, int64_t length, bool addByteOrderMark, bool shouldUseCrLfLineBreaks, bool dryRun);

	virtual string getFileType() = 0;
	virtual void generateOutput(StringBuilder &output) = 0;
};

