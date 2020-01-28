#pragma once

class CountMapEntry {
	string key;
	double amount = 0;
	double maxValue = 0;
	int64_t totalEntries = 0;

public:
	CountMapEntry();
	explicit CountMapEntry(const string &key);

	double getAverage();
	string toString(bool showAverage);

	friend class CountMap;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Maintains counters for e.g. performance measurements.
class CountMap {

	bool shouldRemoveZeroCounts = true;
	map<string, CountMapEntry> countsByName;
	mutex lock;

public:
	void clear();
	void add(const string &s, double deltaAmount, int64_t deltaCount);
	void add(const string &s, double amount) { add(s, amount, +1); }
	void increment(const string &s) { add(s, 1); }
	string toString(bool showAverage);
};

