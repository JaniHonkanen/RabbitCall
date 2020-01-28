#pragma once

class SourceLocation {
	shared_ptr<string> filename;
	LineAndColumnNumber lineAndColumnNumber;

public:
	SourceLocation();
	SourceLocation(const shared_ptr<string> &filename, LineAndColumnNumber lineAndColumnNumber);

	[[nodiscard]] string getFilename() const;
	[[nodiscard]] LineAndColumnNumber getLineAndColumnNumber() const;
	[[nodiscard]] string toString() const;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class ParseException : public exception {

public:
	string message;
	SourceLocation location;

	explicit ParseException(const SourceLocation &location, const char *m) : message(m), location(location) {}
	explicit ParseException(const SourceLocation &location, const string &m) : message(m), location(location) {}

	const char * what() const noexcept override {
		return message.c_str();
	}
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class ErrorList {

	vector<pair<string, string>> errors;
	recursive_mutex lock;

public:
	void addError(const string &error);
	void addError(const SourceLocation &location, const string &error);
	bool hasErrors();
	bool showErrors();
	void addAll(ErrorList *list);
	bool runWithExceptionCheck(const function<void()> &task);
};
