#pragma once


class CppProject {
	Config *config = nullptr;
	ErrorList *errorList = nullptr;
	shared_ptr<TypeMap> typeMap;

	unordered_set<string> outputDirs;
	unordered_map<string, shared_ptr<CppPartition>> partitionsByName;
	vector<shared_ptr<CppPartition>> partitions;
	unordered_map<string, shared_ptr<CppClass>> classesByName;
	set<string> usedCppGlobalIdentifiers;
	LineBreakCounts lineBreakCountsInSource;
	bool processed = false;

public:
	explicit CppProject(Config *config, ErrorList *errorList);
	DISABLE_COPY_AND_MOVE(CppProject);
	~CppProject();

	Config * getConfig() { return config; }
	ErrorList * getErrorList() { return errorList; }
	TypeMap * getTypeMap() { return typeMap.get(); }

	bool isFileInsideAnyOutputDir(const string &path);
	void checkFileNotInsideAnotherPartition(const Path &path, CppPartition *partition);
	
	// Appends a number to the generated class/function name in case there would be multiple classes/functions with the same name
	string reserveCppGlobalIdentifier(const string &basename);

	shared_ptr<CppPartition> createPartitionIfDoesNotExist(const Config::Partition &partitionConfig);

	bool addClassIfDoesNotExist(shared_ptr<CppClass> clazz, CppPartition *partition);
	shared_ptr<CppClass> getClassByCppNameIfExists(const string &cppName);
	void addClassDependency(CppClass *dependent, CppClass *dependee);

	void applyFileParseResult(shared_ptr<CppParsedFile> parsedFile, CppPartition *partition);

	void linkParsedFiles();
	void generateOutputFiles();
	CppStatistics calculateTotalStatistics();
	void processProject();

	void forEachPartition(const function<void(CppPartition *)> &c);
};


