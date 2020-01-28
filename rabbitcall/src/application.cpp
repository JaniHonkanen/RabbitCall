#include "pch.h"


unique_ptr<Application> app;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

StopWatch Application::createStopWatchForPerformanceMeasurement() {
	return StopWatch(&performanceCounts);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int Application::run(int argc, char *argv[]) {
	int returnCode = 0;
	errorList.reset(new ErrorList());

	errorList->runWithExceptionCheck([&]() {
		config.reset(new Config(argc, argv, errorList.get()));
		if (errorList->hasErrors()) {
			return;
		}

		cppParseUtil.reset(new CppParseUtil());
		CppTokenizerTables::instance.reset(new CppTokenizerTables());
		CppTokenizerTables::instance->init();

		if (!config->perfTestMode) {
			// Run with lower process priority to avoid making the IDE slow while building.
			setProcessPriorityToLow();
		}
		
		threadPool.reset(new ThreadPool(config->maxThreads));
		threadPool->start();

		int rounds = 1;
		if (config->perfTestMode) {
			LOG_INFO("Running performance test");
			rounds = 100;
		}
		for (int round = 0; round < rounds; round++) {
			double startTime = getTimeSeconds();

			unique_ptr<CppProject> cppProject = make_unique<CppProject>(config.get(), errorList.get());
			cppProject->processProject();

			if (errorList->hasErrors()) {
				break;
			}
			
			CppStatistics stats = cppProject->calculateTotalStatistics();
			StopWatch stopWatch = createStopWatchForPerformanceMeasurement();
			cppProject.reset(nullptr);
			stopWatch.mark("clean-up");

			double secondsElapsed = getTimeSeconds() - startTime;
			performanceCounts.add("total time", secondsElapsed);
			LOG_INFO(sb() << "Processed " << (stats.numSourceBytes / 1048576.0) << " MB (" << stats.numSourceFiles << " files, " << stats.numClasses << " classes, " << stats.numFunctions << " functions) in " << secondsElapsed << " seconds (" << (stats.numSourceBytes / 1048576.0 / secondsElapsed) << " MB/s)");
			LOG_INFO("Cross-language bindings generated successfully.");
		}

		if (config->perfTestMode) {
			LOG(LogLevel::off, sb() << "Performance test results:\n" << performanceCounts.toString(true));
		}
	});

	if (errorList->hasErrors()) {
		errorList->showErrors();
		returnCode = applicationErrorReturnCode;
	}
	return returnCode;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int main(int argc, char *argv[]) {
	logger.reset(new Logger());
	app.reset(new Application());
	int returnValue = app->run(argc, argv);
	app.reset();
	exit(returnValue); // Call exit() to stop threads that might not have been stopped successfully.
	return returnValue; // Not reached but main() must return an int on Linux.
}

