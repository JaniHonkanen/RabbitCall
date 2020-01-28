#include "pch.h"

void ConfigParameterMap::add(const string &name, const string &value, const string &errorMessageForDuplicateParameter) {
	if (valuesByName.find(name) != valuesByName.end()) EXC(sb() << errorMessageForDuplicateParameter << ": " << name);
	valuesByName[name] = value;
	allNames.push_back(name);
}

void ConfigParameterMap::addAll(ConfigParameterMap &map, const string &errorMessageForDuplicateParameter) {
	for (auto &entry : map.valuesByName) {
		add(entry.first, entry.second, errorMessageForDuplicateParameter);
	}
	usedNames.insert(map.usedNames.begin(), map.usedNames.end());
}

bool ConfigParameterMap::hasParameterAndMarkUsed(const string &name) {
	if (valuesByName.find(name) != valuesByName.end()) {
		usedNames.insert(name);
		return true;
	}
	return false;
}

string ConfigParameterMap::getIfExistsAndMarkUsed(const string &name) {
	if (!hasParameterAndMarkUsed(name)) return "";
	usedNames.insert(name);
	return valuesByName.at(name);
}

string ConfigParameterMap::getOrThrowAndMarkUsed(const string &name) {
	if (!hasParameterAndMarkUsed(name)) EXC(sb() << "Missing configuration parameter: " << name);
	usedNames.insert(name);
	return valuesByName.at(name);
}

vector<string> ConfigParameterMap::getUnusedNames() {
	vector<string> result;
	for (const string &name : allNames) {
		if (!contains(usedNames, name)) result.push_back(name);
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Path Config::convertProjectPathToAbsolutePath(const string &path) {
	if (path.empty()) return Path();
	return canonicalizePath(combinePathIfRelative(cppProjectDir, Path(path)));
}

void Config::readPartitions(tinyxml2::XMLElement *xmlPartitions, const shared_ptr<string> &configFileName) {
	string partitionElementName = "partition";
	tinyxml2::XMLElement *xmlPartition = xmlPartitions->FirstChildElement(partitionElementName.c_str());
	while (xmlPartition) {
		Partition p = {};
		p.name = xmlPartition->GetAttributeOrThrow("name");

		tinyxml2::XMLElement *cppSourceFilesElement = xmlPartition->FirstChildElement("cppSourceFiles");
		if (cppSourceFilesElement) {
			p.cppSourceDir = convertProjectPathToAbsolutePath(cppSourceFilesElement->GetAttributeOrThrow("dir"));
			p.cppSourceFileSet.initFromXml(cppSourceFilesElement);
		}

		partitions.push_back(p);
		xmlPartition = xmlPartition->NextSiblingElement(partitionElementName.c_str());
	}
}

void Config::readTypeMappings(tinyxml2::XMLElement *xmlTypeMappings, const shared_ptr<string> &configFileName) {
	string typeElementName = "type";
	tinyxml2::XMLElement *xmlType = xmlTypeMappings->FirstChildElement(typeElementName.c_str());
	while (xmlType) {
		TypeMapping m = {};
		m.typeNames.cppType = xmlType->GetAttributeOrThrow("cpp");
		m.typeNames.csType = xmlType->GetAttributeOrThrow("cs");
		m.typeNames.hlslType = xmlType->GetAttributeIfExists("hlsl");
		m.typeNames.glslType = xmlType->GetAttributeIfExists("glsl");
		m.marshal = xmlType->GetAttributeIfExists("marshal");
		m.charType = xmlType->GetAttributeIfExists("charType");
		m.sourceLocation = SourceLocation(configFileName, LineAndColumnNumber(xmlType->getLineNumber(), 0));

		auto parseXmlTypeIntegerParameter = [&](string name) {
			string s = nullToEmpty(xmlType->Attribute(name.c_str()));
			if (!s.empty()) {
				int64_t size;
				if (tryParseInt64(s, &size)) {
					return (size_t)size;
				}
				else {
					EXC(sb() << "Invalid " << name << " attribute for type " << m.typeNames.cppType << ": " << s);
				}
			}
			return (size_t)0;
		};

		m.size = parseXmlTypeIntegerParameter("size");
		m.alignment = parseXmlTypeIntegerParameter("alignment");

		typeMappings.push_back(m);
		xmlType = xmlType->NextSiblingElement(typeElementName.c_str());
	}
}

Config::Config(int argc, char *argv[], ErrorList *errorList) {

	Path configFile;
	try {
		logger->setMessagePrefix(programName); // Use the program name as log message prefix although it may be overridden later after the config has been read.

		ConfigParameterMap cmdLineParams;
		{
			string paramName;
			auto addCurrentCmdLineParamToMap = [&](string value) {
				if (!paramName.empty()) {
					cmdLineParams.add(paramName, value, "Command-line parameter defined multiple times");
					paramName = "";
				}
			};
			
			for (int i = 1; i < argc;) {
				string p = argv[i];

				if (boost::starts_with(p, "-")) {
					if (!paramName.empty()) addCurrentCmdLineParamToMap("");
					paramName = p.substr(1);
				}
				else {
					if (!paramName.empty()) {
						addCurrentCmdLineParamToMap(p);
					}
					else {
						EXC(sb() << "Expected command-line parameter name: " << p);
					}
				}

				i++;
			}
		}

		configFile = canonicalizePath(Path(cmdLineParams.getOrThrowAndMarkUsed("configFile")));
		LOG_DEBUG(sb() << "Running " << programName << " for config file: " << configFile.toString());
		if (!exists(configFile.path())) EXC(sb() << "Config file does not exist: " << configFile.toString());
		if (!configFile.path().has_parent_path()) EXC(sb() << "Could not determine config file's directory: " << configFile.toString());
		shared_ptr<string> configFileName = make_shared<string>(configFile.toString());

		cppProjectDir = configFile.path().parent_path();
		LOG_DEBUG(sb() << "C++ project dir: " << cppProjectDir.toString());

		tinyxml2::XMLDocument xmlConfig;
		if (xmlConfig.LoadFile(configFile.toString().c_str())) EXC(sb() << xmlConfig.ErrorStr());
		tinyxml2::XMLElement *xmlRoot = xmlConfig.FirstChildElementOrThrow("project");
		ConfigParameterMap params;
		xmlRoot->forEachChildElement([&](tinyxml2::XMLElement *element) {
			string name = element->Name();
			if (name == "typeMappings") {
				readTypeMappings(element, configFileName);
			}
			else if (name == "partitions") {
				readPartitions(element, configFileName);
			}
			else if (name == "outputDir") {
				string type = element->GetAttributeOrThrow("type");
				if (contains(outputDirsByType, type)) EXC(sb() << "Output dir of type '" << type << "' defined multiple times.");
				OutputDir outputDir;
				outputDir.path = convertProjectPathToAbsolutePath(element->GetText());
				outputDir.shouldWriteByteOrderMark = element->GetAttributeIfExists("bom") == "true";
				outputDirsByType[type] = outputDir;
			}
			else {
				params.add(name, element->GetText(), sb() << "Parameter defined multiple times in configuration file (" << configFile.toString() << ")");
			}
		});
		params.addAll(cmdLineParams, "Parameter defined both in command-line parameters and configuration file");

		dryRunMode = params.hasParameterAndMarkUsed("dryRun");
		perfTestMode = params.hasParameterAndMarkUsed("perfTest");

		logger->setLogLevel(Logger::parseLogLevel(params.getOrThrowAndMarkUsed("logLevel")));
		logger->setMessagePrefix(params.getIfExistsAndMarkUsed("logMessagePrefix"));

		string logFile = params.getIfExistsAndMarkUsed("logFile");
		if (!logFile.empty()) {
			logFile = canonicalizePath(combinePathIfRelative(cppProjectDir, Path(logFile))).toString();
			LOG_DEBUG(sb() << "Writing log: " << logFile);
			logger->setLogFile(logFile);
		}

		auto getIntegerConfigValue = [&](const string &name) {
			string valueText = params.getIfExistsAndMarkUsed(name);
			if (valueText.empty()) EXC(sb() << "Missing configuration parameter: " << name);
			int64_t value;
			if (!tryParseInt64(valueText, &value)) {
				EXC(sb() << "Invalid configuration parameter for " << name << " (should be an integer): " << valueText);
			}
			return value;
		};

		cppLibraryFile = params.getOrThrowAndMarkUsed("cppLibraryFile");
		includeSourceHeadersInGeneratedCpp = parseBool(params.getOrThrowAndMarkUsed("includeSourceHeadersInGeneratedCpp"));
		exportKeyword = params.getOrThrowAndMarkUsed("exportKeyword");
		exportKeywordWithParameters = exportKeyword + "P";
		csClassNamePrefix = params.getIfExistsAndMarkUsed("csClassNamePrefix");
		csClassNameSuffix = params.getIfExistsAndMarkUsed("csClassNameSuffix");
		csNamespace = params.getIfExistsAndMarkUsed("csNamespace");
		csGlobalFunctionContainerClass = params.getIfExistsAndMarkUsed("csGlobalFunctionContainerClass");

		maxThreads = (int)getIntegerConfigValue("maxThreads");
		if (maxThreads == 0) maxThreads = 16;
		else if (maxThreads < 1 || maxThreads > 1000) EXC(sb() << "Invalid maxThreads: " << maxThreads);

		generatedCppFilePrologue = params.getOrThrowAndMarkUsed("generatedCppFilePrologue");

		for (const string &name : params.getUnusedNames()) {
			errorList->addError(sb() << "Unknown configuration parameter: " << name);
		}
		
		allowedCsFixedSizeArrayTypes.insert("bool");
		allowedCsFixedSizeArrayTypes.insert("byte");
		allowedCsFixedSizeArrayTypes.insert("short");
		allowedCsFixedSizeArrayTypes.insert("int");
		allowedCsFixedSizeArrayTypes.insert("long");
		allowedCsFixedSizeArrayTypes.insert("char");
		allowedCsFixedSizeArrayTypes.insert("sbyte");
		allowedCsFixedSizeArrayTypes.insert("ushort");
		allowedCsFixedSizeArrayTypes.insert("uint");
		allowedCsFixedSizeArrayTypes.insert("ulong");
		allowedCsFixedSizeArrayTypes.insert("float");
		allowedCsFixedSizeArrayTypes.insert("double");
	}
	catch (exception &e) {
		throw ParseException(SourceLocation(make_shared<string>(configFile.toString()), LineAndColumnNumber()), e.what());
	}
}

bool Config::tryGetOutputDirByType(const string &type, OutputDir *result) {
	auto iter = outputDirsByType.find(type);
	if (iter == outputDirsByType.end()) return false;
	*result = iter->second;
	return true;
}

