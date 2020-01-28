#pragma once

class CppProject;

struct CppOutputFile {
	Path file;
	bool shouldWriteByteOrderMark = false;
};

// The source files may be split to multiple partitions if the source code size is large.
class CppPartition {
	string partitionName;
	Config *config = nullptr;
	ErrorList *errorList = nullptr;
	CppProject *cppProject = nullptr;
	TypeMap *typeMap = nullptr;
	Path cppSourceDirIfExists;
	FileSet cppSourceFileSet;
	shared_ptr<CppFuncVarParser> funcVarParser;

	CppSourceDirectory rootDir;
	vector<shared_ptr<CppParsedFile>> parsedFiles;
	vector<shared_ptr<CppFuncVar>> globalFunctions;
	vector<shared_ptr<CppClass>> classes;
	set<string> headerFilesUsed;
	map<string, shared_ptr<CppOutputFile>> outputFilesByType;
	CppStatistics statistics;

public:
	explicit CppPartition(const Config::Partition &partitionConfig, CppProject *cppProject);
	DISABLE_COPY_AND_MOVE(CppPartition);
	~CppPartition();

	string getName() { return partitionName; }
	Config * getConfig() { return config; }
	ErrorList * getErrorList() { return errorList; }
	CppProject * getCppProject() { return cppProject; }
	TypeMap * getTypeMap() { return typeMap; }
	Path getCppSourceDirIfExists() { return cppSourceDirIfExists; }
	FileSet * getCppSourceFileSet() { return &cppSourceFileSet; }
	shared_ptr<CppOutputFile> getOutputFileByTypeIfExists(const string &fileType);

	void addClass(shared_ptr<CppClass> clazz);
	void applyFileParseResult(shared_ptr<CppParsedFile> parsedFile);
	set<string> getSourceHeaderFilesUsed() { return headerFilesUsed; }
	CppStatistics * getStatistics() { return &statistics; }

	void startFindSourceFiles(ThreadPool::TaskSet *tasks);
	void finishFindSourceFiles();
	bool containsPath(const Path &path);
	
	void resolveClassMembers();
	void resolveSuperclasses();
	void buildStructLayout(CppClass *c);
	void buildStructLayouts();
	void resolveFunctions();
	void linkParsedFiles();

	string getEnumReflectionVariableName(const CppClass *clazz, bool qualified);

	void forEachParsedFile(const function<void(const shared_ptr<CppParsedFile> &)> &c);
	void forEachGlobalFunction(const function<void(CppFuncVar *declaration)> &c);
	void forEachClass(const function<void(CppClass *clazz)> &c);
	void forEachEnum(const function<void(CppClass *clazz)> &c);
	void forEachSuperclassDepthFirst(CppClass *clazz, bool recursive, CppAccessModifier stricteningAccessModifier, const function<void(CppClass *, CppAccessModifier)> &c);
	void forEachMemberFunction(CppClass *clazz, bool includeSuperclasses, CppAccessModifier stricteningAccessModifier, const function<void(CppClass *, CppFuncVar *, CppAccessModifier)> &c);
	void forEachMemberVariable(CppClass *clazz, bool includeSuperclasses, CppAccessModifier stricteningAccessModifier, const function<void(CppClass *, CppStructLayout::Field *, CppAccessModifier)> &c);
	string getFunctionEntryPoint(CppFuncVar * func, CppClass *enclosingClassIfNotGlobal);
	vector<CppFuncVar *> getAccessibleMemberFunctions(CppClass *clazz);
};
