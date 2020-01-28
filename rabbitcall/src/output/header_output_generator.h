#pragma once

class HeaderOutputGenerator : public OutputFileGenerator {

	void outputEnum(CppClass *clazz, StringBuilder &output);
	
public:
	HeaderOutputGenerator() = default;
	DISABLE_COPY_AND_MOVE(HeaderOutputGenerator);
	virtual ~HeaderOutputGenerator() = default;

	string getFileType() override { return "h"; }
	void generateOutput(StringBuilder &output) override;
};
