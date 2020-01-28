#pragma once

// A mapping between type names in different programming languanges.
class TypeMapping {
public:
	TypeNamesByLanguage typeNames;
	string marshal;
	string charType;
	size_t size = 0;
	size_t alignment = 0;

	bool isPassByValue = true;
	bool isString = false;
	SourceLocation sourceLocation;
	string partitionName;

	string csMarshalAttributeIfUsed;
	string cppTransferTypeName; // C++ type used to send/receive to/from C#

	function<string(string)> transformReturnValueInCs = [](string v) { return v; };

	TypeMapping() = default;

	bool hasDefinedSize() { return size != 0; }
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// A trie structure for finding type mappings by multipart names, e.g. "signed int".
// Includes all partial namespace variations, e.g. ns1::ns2::mytype has entries ns1::ns2::mytype, ns2::mytype, mytype.
class TypeMappingTrieNode {
	unordered_map<string, shared_ptr<TypeMappingTrieNode>> subNodesByName;
	vector<shared_ptr<TypeMapping>> types;

public:
	TypeMappingTrieNode * getSubNodeByNameIfExists(const string &name);
	TypeMapping * getTypeIfUnique();
	bool isTypeAmbiguous();
	TypeMapping * getTypeMapping(const string &name, int64_t startIndex, const SourceLocation &sourceLocation);

	void addTypeMapping(vector<string> nameParts, int64_t namePartIndex, shared_ptr<TypeMapping> type);
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Contains all mappings between type names in different programming languages.
class TypeMap {

	Config *config = nullptr;
	
	unordered_map<string, shared_ptr<TypeMapping>> typeMappingsByFullyQualifiedCppType;
	TypeMappingTrieNode typeMappingsByPartiallyQualifiedSuffixes;
	vector<shared_ptr<TypeMapping>> typeMappings;

public:
	bool allTypesIdentified = false; // True when all custom type names (classes etc.) are known, i.e. all input files have been scanned, which allows identifying type names in variable/function declarations.

	explicit TypeMap(Config *config);
	DISABLE_COPY_AND_MOVE(TypeMap);
	~TypeMap() = default;

	void addTypeMapping(shared_ptr<TypeMapping> mapping);
	void checkAllTypesIdentified();
	TypeMappingTrieNode * getTypeMappingRootNodeAndCheckAllTypesAvailable();
	TypeMapping * getTypeMappingByPossiblyPartiallyQualifiedCppTypeIfExists(const string &name, const SourceLocation &sourceLocation);
	TypeMapping * getTypeMappingByPossiblyPartiallyQualifiedCppTypeOrThrow(const string &name, const SourceLocation &sourceLocation);
	void forEachTypeMapping(const function<void(TypeMapping *)> &c);

};
