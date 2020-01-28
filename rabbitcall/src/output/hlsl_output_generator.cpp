#include "pch.h"


void HlslOutputGenerator::outputStructOrConstantBuffer(CppClass *clazz, StringBuilder &output) {
	if (!clazz->typeMapping->isPassByValue) return;
	CppExportParameters::HlslParameters hlslParameters = clazz->exportParameters->hlslParameters;

	output.appendLine("");
	outputComment(clazz->comment.get(), output);
	{
		output.appendIndent();
		output << (hlslParameters.isCBuffer ? "cbuffer" : "struct");
		output << " ";
		output << clazz->typeMapping->typeNames.hlslType;

		string cbufferRegister = hlslParameters.registerName;
		if (!cbufferRegister.empty()) {
			output << " : register(" << cbufferRegister << ")";
		}
		
		output << " {\n";
	}
	output.changeIndent(+1);
	int64_t structSize = clazz->getLayoutOrThrow()->size;
	if (hlslParameters.isCBuffer && structSize % 16 != 0) throw ParseException(clazz->sourceLocation, sb() << "Constant buffer size must be divisible by 16 bytes (was " << clazz->getLayoutOrThrow()->size << "), also 16-byte alignment would fix this");

	int64_t offset = 0;
	for (auto &field : clazz->getLayoutOrThrow()->fields) {
		if (field.pointerDepth > 0) throw ParseException(field.sourceLocation, "Pointer not allowed in a struct that is exported to HLSL");
		if (field.elementSize % 4 != 0) throw ParseException(field.sourceLocation, "HLSL struct member size must be a multiple of 4 bytes");

		string typeName = field.typeNames.hlslType;
		if (typeName.empty()) throw ParseException(field.sourceLocation, sb() << "HLSL data type not defined for C++ type (in configuration or in exported struct): " << field.typeNames.cppType);

		bool isArray = field.arraySize > 0;
		if (isArray && field.elementSize % 16 != 0) throw ParseException(field.sourceLocation, "Array not allowed in a struct that is exported to HLSL, unless the element size is divisible by 16 bytes (because HLSL would round the element size up to 16 bytes anyway)");
		int64_t size = field.elementSize * max((int64_t)1, field.arraySize);
		
		int64_t nextOffset = offset + size;
		// If the variable would cross a 16-byte boundary, align it to the next boundary.
		if (offset % 16 != 0 && nextOffset % 16 != 0 && offset / 16 != nextOffset / 16) {
			offset += 16 - offset % 16;
		}

		if (offset != field.offset) throw ParseException(field.sourceLocation, sb() << "Field '" << field.name << "' offset would be different in HLSL (" << offset << ") than in C++ (" << field.offset << ")");

		outputComment(field.comment.get(), output);

		output.appendIndent();
		output << typeName;
		output << " ";
		output << hlslParameters.fieldPrefix;
		output << field.name;
		if (isArray) {
			output << "[" << field.arraySize << "]";
		}
		output << ";\n";

		offset += size;
	}
	output.changeIndent(-1);
	output.appendLine("};");
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void HlslOutputGenerator::outputEnum(CppClass *clazz, StringBuilder &output) {
	CppExportParameters::HlslParameters hlslParameters = clazz->exportParameters->hlslParameters;
	string prefix = hlslParameters.fieldPrefix;
	if (prefix.empty()) {
		prefix = sb() << clazz->typeNames.cppType << "_";
	}

	output.appendLine("");

	if (outputComment(clazz->comment.get(), output)) {
		output.appendLine("");
	}

	for (auto &field : clazz->enumFields) {
		if (field.value.empty()) throw ParseException(field.sourceLocation, "Uninitialized enum fields not supported in HLSL");
		outputComment(field.comment.get(), output);
		output.appendIndent() << "static const int " << prefix << field.name << " = " << field.value << ";\n";
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void HlslOutputGenerator::generateOutput(StringBuilder &output) {
	StopWatch stopWatch = app->createStopWatchForPerformanceMeasurement();

	partition->forEachClass([&](CppClass *clazz) {
		if (clazz->exportParameters->hlslParameters.exportHlsl) {
			if (clazz->classType == CppClassDeclarationType::CLASS || clazz->classType == CppClassDeclarationType::STRUCT) {
				outputStructOrConstantBuffer(clazz, output);
			}
			else if (clazz->classType == CppClassDeclarationType::ENUM) {
				outputEnum(clazz, output);
			}
		}
	});

	stopWatch.mark("output / generate .hlsl");
}
