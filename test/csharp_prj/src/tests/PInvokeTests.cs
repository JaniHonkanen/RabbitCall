using System;
using System.Collections.Generic;
using System.Linq;
using System.Numerics;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Security;
using System.Text;
using CsNamespace;

public unsafe class PInvokeTests {

	long defaultRounds = 1000;
	public double dummyResult = 0;

	[SuppressUnmanagedCodeSecurity, DllImport(RabbitCallApi.cppLibraryFile, EntryPoint = "pinvokeTest_vector4Param_direct")]
	static extern void pinvokeTest_vector4Param_direct(Vector4 v);

	[SuppressUnmanagedCodeSecurity, DllImport(RabbitCallApi.cppLibraryFile, EntryPoint = "pinvokeTest_vector4Param_ptr")]
	static extern void pinvokeTest_vector4Param_ptr(Vector4 *v);

	[SuppressUnmanagedCodeSecurity, DllImport(RabbitCallApi.cppLibraryFile, EntryPoint = "pinvokeTest_vector4Return_direct")]
	static extern Vector4 pinvokeTest_vector4Return_direct();

	[SuppressUnmanagedCodeSecurity, DllImport(RabbitCallApi.cppLibraryFile, EntryPoint = "pinvokeTest_vector4Return_ptr")]
	static extern void pinvokeTest_vector4Return_ptr(Vector4 *v);

	[SuppressUnmanagedCodeSecurity, DllImport(RabbitCallApi.cppLibraryFile, EntryPoint = "pinvokeTest_intReturn_direct")]
	static extern double pinvokeTest_intReturn_direct();

	[SuppressUnmanagedCodeSecurity, DllImport(RabbitCallApi.cppLibraryFile, EntryPoint = "pinvokeTest_intReturn_ptr")]
	static extern void pinvokeTest_intReturn_ptr(double *v);

	[SuppressUnmanagedCodeSecurity, DllImport(RabbitCallApi.cppLibraryFile, EntryPoint = "pinvokeTest_doubleReturn_direct")]
	static extern double pinvokeTest_doubleReturn_direct();

	[SuppressUnmanagedCodeSecurity, DllImport(RabbitCallApi.cppLibraryFile, EntryPoint = "pinvokeTest_doubleReturn_ptr")]
	static extern void pinvokeTest_doubleReturn_ptr(double *v);


	public long test_vector4Param_direct() {
		long rounds = defaultRounds;
		for (long i = 0; i < rounds; i++) {
			pinvokeTest_vector4Param_direct(new Vector4(1, 2, 3, 4));
		}
		return rounds;
	}

	public long test_vector4Param_ptr() {
		long rounds = defaultRounds;
		for (long i = 0; i < rounds; i++) {
			Vector4 v = new Vector4(1, 2, 3, 4);
			pinvokeTest_vector4Param_ptr(&v);
		}
		return rounds;
	}

	public long test_vector4Return_direct() {
		long rounds = defaultRounds;
		for (long i = 0; i < rounds; i++) {
			Vector4 v = pinvokeTest_vector4Return_direct();
			dummyResult += v.X + v.Y + v.Z + v.W;
		}
		return rounds;
	}

	public long test_vector4Return_ptr() {
		long rounds = defaultRounds;
		for (long i = 0; i < rounds; i++) {
			Vector4 v;
			pinvokeTest_vector4Return_ptr(&v);
			dummyResult += v.X + v.Y + v.Z + v.W;
		}
		return rounds;
	}

	public long test_intReturn_direct() {
		long rounds = defaultRounds;
		for (long i = 0; i < rounds; i++) {
			dummyResult += pinvokeTest_intReturn_direct();
		}
		return rounds;
	}

	public long test_intReturn_ptr() {
		long rounds = defaultRounds;
		for (long i = 0; i < rounds; i++) {
			double v;
			pinvokeTest_intReturn_ptr(&v);
			dummyResult += v;
		}
		return rounds;
	}

	public long test_doubleReturn_direct() {
		long rounds = defaultRounds;
		for (long i = 0; i < rounds; i++) {
			dummyResult += pinvokeTest_doubleReturn_direct();
		}
		return rounds;
	}

	public long test_doubleReturn_ptr() {
		long rounds = defaultRounds;
		for (long i = 0; i < rounds; i++) {
			double v;
			pinvokeTest_doubleReturn_ptr(&v);
			dummyResult += v;
		}
		return rounds;
	}

	public static List<PerformanceTest> getTests() {
		return PerformanceTests.getTestsFromClassByReflection(new PInvokeTests()).Select(test => {
			test.category = $"PInvoke tests";
			return test;
		}).ToList();
	}

}

