#pragma once

struct TypeNamesByLanguage {
	string cppType;
	string csType;
	string hlslType;
	string glslType;
};

class ConfigParameterMap {
	unordered_map<string, string> valuesByName;
	unordered_set<string> usedNames;
	vector<string> allNames;

public:
	void add(const string &name, const string &value, const string &errorMessageForDuplicateParameter);
	void addAll(ConfigParameterMap &map, const string &errorMessageForDuplicateParameter);
	bool hasParameterAndMarkUsed(const string &name);
	string getIfExistsAndMarkUsed(const string &name);
	string getOrThrowAndMarkUsed(const string &name);
	vector<string> getUnusedNames();
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class Config {

	Path convertProjectPathToAbsolutePath(const string &path);
	void readPartitions(tinyxml2::XMLElement *xmlPartitions, const shared_ptr<string> &configFileName);
	void readTypeMappings(tinyxml2::XMLElement *xmlTypeMappings, const shared_ptr<string> &configFileName);
	
public:
	Config(int argc, char *argv[], ErrorList *errorList);

	struct OutputDir {
		Path path;
		bool shouldWriteByteOrderMark = false;

		bool isValid() const { return !path.empty(); }
	};

	struct Partition {
		string name;
		Path cppSourceDir;
		FileSet cppSourceFileSet;
	};
	
	struct TypeMapping {
		TypeNamesByLanguage typeNames;
		string marshal;
		string charType;
		size_t size;
		size_t alignment;
		SourceLocation sourceLocation;
	};

	string programName = "RabbitCall";
	bool perfTestMode = false;
	bool dryRunMode = false;

	Path cppProjectDir;
	string cppLibraryFile;
	bool includeSourceHeadersInGeneratedCpp = false;
	string exportKeyword;
	string exportKeywordWithParameters;
	string csClassNamePrefix;
	string csClassNameSuffix;
	string csNamespace;
	string csGlobalFunctionContainerClass;
	int maxThreads = 1;
	int64_t pointerSizeBits = 64;
	string generatedCppFilePrologue;
	map<string, OutputDir> outputDirsByType;
	vector<Partition> partitions;
	vector<TypeMapping> typeMappings;

	unordered_set<string> allowedCsFixedSizeArrayTypes;

	string getApiClassName() { return programName + "Api"; }
	int64_t getPointerSizeBytes() { return pointerSizeBits / 8; }
	bool tryGetOutputDirByType(const string &type, OutputDir *result);
	
};


