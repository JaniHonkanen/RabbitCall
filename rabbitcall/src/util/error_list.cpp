#include "pch.h"


SourceLocation::SourceLocation() {
}

SourceLocation::SourceLocation(const shared_ptr<string> &filename, LineAndColumnNumber lineAndColumnNumber)
	: filename(filename), lineAndColumnNumber(lineAndColumnNumber) {
}

string SourceLocation::getFilename() const {
	return filename ? *filename : "";
}

LineAndColumnNumber SourceLocation::getLineAndColumnNumber() const {
	return lineAndColumnNumber;
}

string SourceLocation::toString() const {
	string name = getFilename();
	if (name.empty() && lineAndColumnNumber.line == 0 && lineAndColumnNumber.column == 0) return "";
	return sb() << name << "(" << lineAndColumnNumber.line << "," << lineAndColumnNumber.column << ")";
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void ErrorList::addError(const string &error) {
	lock_guard<recursive_mutex> lockGuard(lock);
	errors.emplace_back("", error);
}

void ErrorList::addError(const SourceLocation &location, const string &error) {
	lock_guard<recursive_mutex> lockGuard(lock);
	errors.emplace_back(location.toString(), error);
}

bool ErrorList::hasErrors() {
	lock_guard<recursive_mutex> lockGuard(lock);
	return !errors.empty();
}

bool ErrorList::showErrors() {
	lock_guard<recursive_mutex> lockGuard(lock);
	if (!errors.empty()) {
		for (auto &error : errors) {
			logger->outputLog(LogLevel::error, error.first, error.second);
		}
		return true;
	}
	return false;
}

void ErrorList::addAll(ErrorList *list) {
	lock_guard<recursive_mutex> lockGuard(lock);
	lock_guard<recursive_mutex> oldListLockGuard(list->lock);

	errors.insert(errors.end(), list->errors.begin(), list->errors.end());
}

bool ErrorList::runWithExceptionCheck(const function<void()> &task) {
	try {
		task();
		return true;
	}
	catch (ParseException &e) {
		addError(e.location, e.what());
	}
	catch (exception &e) {
		addError(SourceLocation(), e.what());
	}
	return false;
}


