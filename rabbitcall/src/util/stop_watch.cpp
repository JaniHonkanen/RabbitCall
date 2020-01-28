#include "pch.h"

void StopWatch::init() {
	startTime = getTimeSeconds();
}

StopWatch::StopWatch() {
	init();
}

StopWatch::StopWatch(CountMap *resultMap)
	: resultMap(resultMap) {
	init();
}

double StopWatch::getTimeAndReset() {
	double currentTime = getTimeSeconds();
	double elapsedTime = currentTime - startTime;
	startTime = currentTime;
	return elapsedTime;
}

void StopWatch::mark(const string &name) {
	if (!resultMap) EXC("StopWatch does not have a result map.");
	resultMap->add(name, getTimeAndReset());
}


