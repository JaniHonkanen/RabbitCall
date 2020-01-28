
using System;
using System.Collections.Generic;
using System.IO;
using System.Numerics;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Security;
using System.Text;
using CsNamespace;

public unsafe class PerformanceTests {

	string resultFile = "perf_test_result.txt";
	long defaultRounds = 10000;
	long outerRounds = 100;
	public double dummyResult = 0;
	NTestClass testObj;
	NCallbackTest callbackTest;

	public long test_emptyFunction() {
		long rounds = defaultRounds;
		for (long i = 0; i < rounds; i++) {
			NGlobal.emptyFunction();
		}
		return rounds;
	}

	public long test_addFloatVectors() {
		long rounds = defaultRounds;
		Vector4 sum = new Vector4();
		for (long i = 0; i < rounds; i++) {
			sum = testObj.addFloatVectors(sum, new Vector4(1, 2, 3, 4));
		}
		dummyResult += sum.X + sum.Y + sum.Z + sum.W;
		return rounds;
	}

	public long test_addFloatVectorsNoexcept() {
		long rounds = defaultRounds;
		Vector4 sum = new Vector4();
		for (long i = 0; i < rounds; i++) {
			sum = testObj.addFloatVectorsNoexcept(sum, new Vector4(1, 2, 3, 4));
		}
		dummyResult += sum.X + sum.Y + sum.Z + sum.W;
		return rounds;
	}

	public long test_setName() {
		long rounds = defaultRounds;
		int sum = 0;
		for (long i = 0; i < rounds; i++) {
			testObj.setName("abc");
		}
		dummyResult += sum;
		return rounds;
	}

	public long test_getName() {
		long rounds = defaultRounds;
		int sum = 0;
		for (long i = 0; i < rounds; i++) { 
			sum += testObj.getName().Length;
		}
		dummyResult += sum;
		return rounds;
	}

	public long test_concatenateStrings() {
		long rounds = defaultRounds;
		int sum = 0;
		for (long i = 0; i < rounds; i++) { 
			sum += testObj.concatenateStrings("abc", "def").Length;
		}
		dummyResult += sum;
		return rounds;
	}

	public long test_replaceCallback() {
		long rounds = defaultRounds;
		for (long i = 0; i < rounds; i++) { 
			callbackTest.setCallback((s1, s2) => s1 + s2);
		}
		return rounds;
	}

	public long test_invokeIntCallback() {
		long rounds = defaultRounds;
		dummyResult += callbackTest.invokeIntCallbackRepeatedly(defaultRounds, (i1, i2) => i1 + i2);
		return rounds;
	}

	public long test_invokeStringParamCallback() {
		long rounds = defaultRounds;
		dummyResult += callbackTest.invokeStringParamCallbackRepeatedly(defaultRounds, s => s.Length);
		return rounds;
	}

	public long test_invokeStringReturnValueCallback() {
		long rounds = defaultRounds;
		dummyResult += callbackTest.invokeStringReturnValueCallbackRepeatedly(defaultRounds, () => "abcdef");
		return rounds;
	}

	public void run() {
		testObj = NTestClass.create("test");
		callbackTest = NGlobal.createCallbackTestInstance();

		Log.write("Running performance tests...");

		StringBuilder primaryResult = new StringBuilder();
		StringBuilder secondaryResult = new StringBuilder();

		primaryResult.Append("Performance test results (CPU cycles):\n");
		primaryResult.Append("\n");

		List<PerformanceTest> tests = new List<PerformanceTest>();

		getTestsFromClassByReflection(this).ForEach(test => {
			test.belongsToPrimaryTests = true;
			tests.Add(test);
		});

		tests.AddRange(PInvokeTests.getTests());
		tests.AddRange(PInvokeStringTests.getTests());

		for (int outerRound = 0; outerRound < outerRounds; outerRound++) {
			foreach (PerformanceTest test in tests) {
				long startTime = NGlobal.getTimeRdtsc();
				long rounds = test.runAndGetRounds();
				long elapsedTime = NGlobal.getTimeRdtsc() - startTime;
				test.times.Add((double)elapsedTime / rounds);
				test.totalTime += elapsedTime;
				test.totalRounds += rounds;
			}
		}

		string lastCategory = null;
		foreach (PerformanceTest test in tests) {
			StringBuilder result = test.belongsToPrimaryTests ? primaryResult : secondaryResult;

			if (test.category != lastCategory) {
				lastCategory = test.category;
				result.Append("\n");
				result.Append($"{lastCategory}:\n");
			}

			result.Append($"{test.name}: {test.sortAndGetMedianTime()}\n");
		}

		Log.write("");
		Log.write(primaryResult.ToString());
		Log.write($"Writing more performance test results to: {Path.GetFullPath(resultFile)}");
		File.WriteAllBytes(resultFile, new UTF8Encoding().GetBytes(primaryResult.ToString() + secondaryResult.ToString()));
	}

	public static List<PerformanceTest> getTestsFromClassByReflection(object obj) {
		List<PerformanceTest> result = new List<PerformanceTest>();
		foreach (MethodInfo methodInfo in obj.GetType().GetMethods()) {
			string prefix = "test_";
			if (methodInfo.Name.StartsWith(prefix)) {
				PerformanceTest test = new PerformanceTest();
				test.name = methodInfo.Name.Substring(prefix.Length);
				test.runAndGetRounds = () => {
					object testResult = methodInfo.Invoke(obj, new object[0]);
					if (testResult == null) throw new Exception("Test should return number of rounds");
					return (long) testResult;
				};
				result.Add(test);
			}
		}
		return result;
	}

}
