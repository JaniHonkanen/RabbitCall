#include "pch.h"

unique_ptr<Logger> logger;

Logger::~Logger() {
	if (file) {
		fclose(file);
		file = nullptr;
	}
}

bool Logger::isLogLevelEnabled(LogLevel level) {
	lock_guard<recursive_mutex> lockGuard(lock);
	return (int)level <= (int)currentLogLevel;
}

void Logger::outputLog(LogLevel level, const string &location, const string &msg) {
	lock_guard<recursive_mutex> lockGuard(lock);
	if (isLogLevelEnabled(level)) {
		string buffer;
		if (!location.empty()) {
			// Write the source code location reference first so that the IDE will navigate to it when clicked.
			buffer += location;
			buffer += ": ";
		}

		// The "error" string must be shown this way after the (optional) location so that Visual Studio will display the message in the error list panel.
		if (level == LogLevel::error) {
			buffer += "error: ";
		}
		
		buffer += messagePrefix;
		buffer += msg;

		cout << buffer << endl;

		if (file) {
			buffer += '\n';
			fwrite(buffer.c_str(), sizeof(char), buffer.length(), file);
			//fflush(file);
		}
	}
}

void Logger::setLogFile(const string &path) {
	lock_guard<recursive_mutex> lockGuard(lock);
	file = fopen(path.c_str(), "wt");
	if (!file) throw runtime_error(sb() << "Error opening log file for writing: " << path);
}

void Logger::setLogLevel(LogLevel level) {
	lock_guard<recursive_mutex> lockGuard(lock);
	currentLogLevel = level;
}

void Logger::setMessagePrefix(const string &prefix) {
	lock_guard<recursive_mutex> lockGuard(lock);
	messagePrefix = prefix.empty() ? "" : (prefix + ": ");
}

LogLevel Logger::parseLogLevel(const string &level) {
	if (level == "off") return LogLevel::off;
	if (level == "error") return LogLevel::error;
	if (level == "info") return LogLevel::info;
	if (level == "detail") return LogLevel::detail;
	if (level == "debug") return LogLevel::debug;
	EXC(sb() << "Unknown log level: " << level);
	return LogLevel::off; // Not reached
}

