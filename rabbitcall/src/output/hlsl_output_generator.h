#pragma once

class HlslOutputGenerator : public OutputFileGenerator {

	void outputStructOrConstantBuffer(CppClass *clazz, StringBuilder &output);
	void outputEnum(CppClass *clazz, StringBuilder &output);

public:
	HlslOutputGenerator() = default;
	DISABLE_COPY_AND_MOVE(HlslOutputGenerator);
	virtual ~HlslOutputGenerator() = default;

	string getFileType() override { return "hlsl"; }
	void generateOutput(StringBuilder &output) override;
};
