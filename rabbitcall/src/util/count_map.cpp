#include "pch.h"

CountMapEntry::CountMapEntry() {
}

CountMapEntry::CountMapEntry(const string &key): key(key) {
}

double CountMapEntry::getAverage() {
	return totalEntries == 0 ? 0 : amount / totalEntries;
}

string CountMapEntry::toString(bool showAverage) {
	StringBuilder b;
	//b << std::fixed;
	b << amount;
	if (showAverage) b << " (avg: " << getAverage() << ")";
	return b;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CountMap::clear() {
	lock_guard<mutex> _(lock);

	countsByName.clear();
}

void CountMap::add(const string &s, double deltaAmount, int64_t deltaCount) {
	lock_guard<mutex> _(lock);

	auto iter = countsByName.find(s);
	if (iter == countsByName.end()) {
		countsByName[s] = CountMapEntry(s);
		iter = countsByName.find(s);
	}
	CountMapEntry &count = iter->second;

	count.amount += deltaAmount;
	count.maxValue = max(count.maxValue, deltaAmount);
	count.totalEntries += deltaCount;

	if (shouldRemoveZeroCounts) {
		if (count.totalEntries == 0) {
			countsByName.erase(count.key);
		}
	}
}

string CountMap::toString(bool showAverage) {
	lock_guard<mutex> _(lock);

	StringBuilder b;
	for (auto &entry : countsByName) {
		b << entry.first << ": " << entry.second.toString(showAverage) << "\n";
	}
	return b;
}


