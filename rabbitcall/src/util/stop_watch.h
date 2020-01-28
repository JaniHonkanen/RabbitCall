#pragma once

class StopWatch {

	CountMap *resultMap = nullptr;
	double startTime = 0;

	void init();
	
public:
	StopWatch();
	explicit StopWatch(CountMap *resultMap);

	double getTimeAndReset();
	void mark(const string &name);

};

