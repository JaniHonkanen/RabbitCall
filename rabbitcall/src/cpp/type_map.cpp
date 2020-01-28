#include "pch.h"

TypeMappingTrieNode * TypeMappingTrieNode::getSubNodeByNameIfExists(const string &name) {
	auto iter = subNodesByName.find(name);
	if (iter == subNodesByName.end()) return nullptr;
	return iter->second.get();
}

TypeMapping * TypeMappingTrieNode::getTypeIfUnique() {
	if (types.size() != 1) return nullptr;
	return types.at(0).get();
}

bool TypeMappingTrieNode::isTypeAmbiguous() {
	return types.size() >= 2;
}

TypeMapping * TypeMappingTrieNode::getTypeMapping(const string &name, int64_t startIndex, const SourceLocation &sourceLocation) {
	if (startIndex >= (int64_t)name.size()) {
		if (isTypeAmbiguous()) throw ParseException(sourceLocation, sb() << "Ambiguous type: " << name << " (try using fully qualified name with namespace prefix)");
		return getTypeIfUnique();
	}

	size_t partEndIndex = name.find(' ', startIndex);
	if (partEndIndex == string::npos) partEndIndex = name.size();
	TypeMappingTrieNode *subNode = getSubNodeByNameIfExists(name.substr(startIndex, partEndIndex - startIndex));
	if (!subNode) {
		return nullptr;
	}
	return subNode->getTypeMapping(name, partEndIndex + 1, sourceLocation);
}

void TypeMappingTrieNode::addTypeMapping(vector<string> nameParts, int64_t namePartIndex, shared_ptr<TypeMapping> type) {
	if (namePartIndex < 0 || namePartIndex > (int64_t)nameParts.size()) EXC(sb() << "Name part index out of range: " << namePartIndex << " for name '" << nameParts << "'");

	if (namePartIndex == nameParts.size()) {
		types.push_back(type);
		//LOG_DEBUG(sb() << "Adding partially-qualified type mapping: " << partialName << " -> " << m->cppType << " (mappings with the same name: " << v.size() << ")");
	}
	else {
		string typePart = nameParts[namePartIndex];
		cppParseUtil->cppNamespaceParser.forEachPartialNamespaceSuffix(typePart, [&](const string &partialName) {
			shared_ptr<TypeMappingTrieNode> &subNode = subNodesByName[partialName]; // Get or create sub-node entry.
			if (!subNode) subNode = make_shared<TypeMappingTrieNode>();
			subNode->addTypeMapping(nameParts, namePartIndex + 1, type);
		});
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TypeMap::TypeMap(Config *config)
	: config(config) {
}

void TypeMap::addTypeMapping(shared_ptr<TypeMapping> mapping) {
	LOG_DEBUG(sb() << "Adding type mapping: " << mapping->typeNames.cppType);
	if (mapping->alignment != 0 && !isPowerOfTwo(mapping->alignment)) EXC(sb() << "Alignment must be a power-of-two (type: " << mapping->typeNames.cppType << ", alignment: " << mapping->alignment << ")");

	mapping->cppTransferTypeName = mapping->typeNames.cppType;

	if (mapping->marshal == "string.utf8") {
		mapping->isString = true;
		mapping->csMarshalAttributeIfUsed = "MarshalAs(UnmanagedType.LPUTF8Str)";
		mapping->transformReturnValueInCs = [mapping](string v) { return (string)(sb() << OUTPUT_INTERNAL_UTIL_CLASS << ".readStringUtf8AndFree(" << v << ")"); };
	}
	else if (mapping->marshal == "string.utf16") {
		mapping->isString = true;
		mapping->csMarshalAttributeIfUsed = "MarshalAs(UnmanagedType.LPWStr)";
		mapping->transformReturnValueInCs = [mapping](string v) { return (string)(sb() << OUTPUT_INTERNAL_UTIL_CLASS << ".readStringUtf16AndFree(" << v << ")"); };
	}

	if (mapping->isString) {
		mapping->cppTransferTypeName = sb() << "const " << mapping->charType << " *";
	}

	if (typeMappingsByFullyQualifiedCppType.find(mapping->typeNames.cppType) != typeMappingsByFullyQualifiedCppType.end()) throw ParseException(mapping->sourceLocation, sb() << "Cannot define the same type multiple times (including config file): " << mapping->typeNames.cppType);
	typeMappingsByFullyQualifiedCppType[mapping->typeNames.cppType] = mapping;

	vector<string> typeParts;
	boost::algorithm::split(typeParts, mapping->typeNames.cppType, boost::algorithm::is_space());
	typeMappingsByPartiallyQualifiedSuffixes.addTypeMapping(typeParts, 0, mapping);

	typeMappings.push_back(mapping);
}

void TypeMap::checkAllTypesIdentified() {
	if (!allTypesIdentified) EXC("Should not query type information before all input files have been scanned and type identified.");
}

TypeMappingTrieNode * TypeMap::getTypeMappingRootNodeAndCheckAllTypesAvailable() {
	checkAllTypesIdentified();
	return &typeMappingsByPartiallyQualifiedSuffixes;
}

TypeMapping * TypeMap::getTypeMappingByPossiblyPartiallyQualifiedCppTypeIfExists(const string &name, const SourceLocation &sourceLocation) {
	checkAllTypesIdentified();
	return typeMappingsByPartiallyQualifiedSuffixes.getTypeMapping(name, 0, sourceLocation);
}

TypeMapping * TypeMap::getTypeMappingByPossiblyPartiallyQualifiedCppTypeOrThrow(const string &name, const SourceLocation &sourceLocation) {
	checkAllTypesIdentified();
	TypeMapping *mapping = getTypeMappingByPossiblyPartiallyQualifiedCppTypeIfExists(name, sourceLocation);
	if (!mapping) throw ParseException(sourceLocation, sb() << "Unknown type: " << name << " (maybe missing type mapping in configuration, or missing " << config->exportKeyword << " keyword at type declaration?)");
	return mapping;
}

void TypeMap::forEachTypeMapping(const function<void(TypeMapping *)> &c) {
	for (const auto &mapping : typeMappings) {
		c(mapping.get());
	}
}

