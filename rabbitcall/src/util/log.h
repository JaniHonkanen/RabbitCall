#pragma once

enum class LogLevel {
	off, error, info, detail, debug
};

class Logger {
	FILE *file = nullptr;
	LogLevel currentLogLevel = LogLevel::info;
	string messagePrefix;
	recursive_mutex lock;

public:
	~Logger();

	bool isLogLevelEnabled(LogLevel level);
	void outputLog(LogLevel level, const string &location, const string &msg);
	void setLogFile(const string &path);
	void setLogLevel(LogLevel level);
	void setMessagePrefix(const string &prefix);
	
	static LogLevel parseLogLevel(const string &level);
};
extern unique_ptr<Logger> logger;

#define LOG(level, msg) if (logger->isLogLevelEnabled(level)) logger->outputLog(level, "", msg)
#define LOG_ERROR(msg) LOG(LogLevel::error, msg)
#define LOG_INFO(msg) LOG(LogLevel::info, msg)
#define LOG_DETAIL(msg) LOG(LogLevel::detail, msg)
#define LOG_DEBUG(msg) LOG(LogLevel::debug, msg)

