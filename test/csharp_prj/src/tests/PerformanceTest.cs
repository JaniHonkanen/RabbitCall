using System;
using System.Collections.Generic;
using System.Text;

public class PerformanceTest {

	public string name;
	public string category;
	public Func<long> runAndGetRounds;
	public List<double> times = new List<double>();
	public double totalTime = 0;
	public long totalRounds = 0;
	public bool belongsToPrimaryTests = false;

	public double sortAndGetMedianTime() {
		if (times.Count == 0) return 0;
		times.Sort((v1, v2) => v1.CompareTo(v2));
		return times[times.Count / 2];
	}

}
