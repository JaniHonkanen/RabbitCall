#include "pch.h"

std::string readFile(const std::string &file) {
	std::ifstream is(file, std::ifstream::binary);
	if (!is) throw std::runtime_error(std::string("Error opening file: ") + file);
	is.seekg(0, std::ifstream::end);
	std::ifstream::pos_type length = is.tellg();
	is.seekg(0, std::ifstream::beg);
	std::string result;
	result.resize(length);
	is.read(result.data(), length);
	if (!is) throw std::runtime_error(std::string("Error reading file: ") + file);
	return result;
}

std::string combinePath(const std::string &path1, const std::string &path2) {
	if (!path1.empty()) {
		char lastChar = path1[path1.size() - 1];
		if (lastChar != '/' && lastChar != '\\') {
			return path1 + "/" + path2;
		}
	}
	return path1 + path2;
}


