#pragma once


class Application {

public:

	unique_ptr<Config> config;
	unique_ptr<ThreadPool> threadPool;
	unique_ptr<CppProject> cppProject;
	unique_ptr<ErrorList> errorList;
	CountMap performanceCounts;

	StopWatch createStopWatchForPerformanceMeasurement();
	
	int run(int argc, char *argv[]);
};

extern unique_ptr<Application> app;

