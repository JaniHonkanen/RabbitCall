using System;
using System.Collections.Generic;
using System.Linq;
using System.Numerics;
using System.Reflection;
using System.Runtime.CompilerServices;
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

	[SuppressUnmanagedCodeSecurity, DllImport(RabbitCallApi.cppLibraryFile, EntryPoint = "pinvokeTest_vector2Param_direct")]
	static extern void pinvokeTest_vector2Param_direct(Vector2 v);

	[SuppressUnmanagedCodeSecurity, DllImport(RabbitCallApi.cppLibraryFile, EntryPoint = "pinvokeTest_vector2Param_ptr")]
	static extern void pinvokeTest_vector2Param_ptr(Vector2 *v);

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

	[SuppressUnmanagedCodeSecurity, DllImport(RabbitCallApi.cppLibraryFile, EntryPoint = "pinvokeTest_invokeGivenStaticCallback")]
	static extern int pinvokeTest_invokeGivenStaticCallback(IntPtr cb, int v);

	[SuppressUnmanagedCodeSecurity, DllImport(RabbitCallApi.cppLibraryFile, EntryPoint = "pinvokeTest_setStaticCallback")]
	static extern int pinvokeTest_setStaticCallback(IntPtr cb);

	[SuppressUnmanagedCodeSecurity, DllImport(RabbitCallApi.cppLibraryFile, EntryPoint = "pinvokeTest_invokeStoredStaticCallback")]
	static extern int pinvokeTest_invokeStoredStaticCallback(int v);

	[SuppressUnmanagedCodeSecurity, DllImport(RabbitCallApi.cppLibraryFile, EntryPoint = "pinvokeTest_invokeGivenDynamicCallback")]
	static extern int pinvokeTest_invokeGivenDynamicCallback(IntPtr cb, IntPtr obj, int v);

	[SuppressUnmanagedCodeSecurity, DllImport(RabbitCallApi.cppLibraryFile, EntryPoint = "pinvokeTest_setDynamicCallback")]
	static extern int pinvokeTest_setDynamicCallback(IntPtr cb);

	[SuppressUnmanagedCodeSecurity, DllImport(RabbitCallApi.cppLibraryFile, EntryPoint = "pinvokeTest_invokeStoredDynamicCallback")]
	static extern int pinvokeTest_invokeStoredDynamicCallback(IntPtr obj, int v);

	delegate int TestStaticDelegate(int v);
	TestStaticDelegate testStaticDelegate;
	IntPtr testStaticDelegatePtr;
	#if ENABLE_IL2CPP
	[AOT.MonoPInvokeCallback(typeof(TestStaticDelegate))]
	#endif
	static int testStaticCallback(int v) {
		return v;
	}

	delegate int TestDynamicDelegate(IntPtr obj, int v);
	TestDynamicDelegate testDynamicDelegate;
	IntPtr testDynamicDelegatePtr;
	#if ENABLE_IL2CPP
	[AOT.MonoPInvokeCallback(typeof(TestDynamicDelegate))]
	#endif
	static int testDynamicCallback(IntPtr obj, int v) {
		return ((TestStaticDelegate)GCHandle.FromIntPtr(obj).Target)(v);
	}

	public PInvokeTests() {
		testStaticDelegate = testStaticCallback;
		testStaticDelegatePtr = Marshal.GetFunctionPointerForDelegate(testStaticDelegate);
		testDynamicDelegate = testDynamicCallback;
		testDynamicDelegatePtr = Marshal.GetFunctionPointerForDelegate(testDynamicDelegate);
	}

	public long test_param_vector4_direct() {
		long rounds = defaultRounds;
		for (long i = 0; i < rounds; i++) {
			pinvokeTest_vector4Param_direct(new Vector4(1, 2, 3, 4));
		}
		return rounds;
	}

	public long test_param_vector4_ptr() {
		long rounds = defaultRounds;
		for (long i = 0; i < rounds; i++) {
			Vector4 v = new Vector4(1, 2, 3, 4);
			pinvokeTest_vector4Param_ptr(&v);
		}
		return rounds;
	}

	public long test_param_vector2_direct() {
		long rounds = defaultRounds;
		for (long i = 0; i < rounds; i++) {
			pinvokeTest_vector2Param_direct(new Vector2(1, 2));
		}
		return rounds;
	}

	public long test_param_vector2_ptr() {
		long rounds = defaultRounds;
		for (long i = 0; i < rounds; i++) {
			Vector2 v = new Vector2(1, 2);
			pinvokeTest_vector2Param_ptr(&v);
		}
		return rounds;
	}

	public long test_return_vector4_direct() {
		long rounds = defaultRounds;
		for (long i = 0; i < rounds; i++) {
			Vector4 v = pinvokeTest_vector4Return_direct();
			dummyResult += v.X + v.Y + v.Z + v.W;
		}
		return rounds;
	}

	public long test_return_vector4_ptr() {
		long rounds = defaultRounds;
		for (long i = 0; i < rounds; i++) {
			Vector4 v;
			pinvokeTest_vector4Return_ptr(&v);
			dummyResult += v.X + v.Y + v.Z + v.W;
		}
		return rounds;
	}

	public long test_return_int_direct() {
		long rounds = defaultRounds;
		for (long i = 0; i < rounds; i++) {
			dummyResult += pinvokeTest_intReturn_direct();
		}
		return rounds;
	}

	public long test_return_int_ptr() {
		long rounds = defaultRounds;
		for (long i = 0; i < rounds; i++) {
			double v;
			pinvokeTest_intReturn_ptr(&v);
			dummyResult += v;
		}
		return rounds;
	}

	public long test_return_double_direct() {
		long rounds = defaultRounds;
		for (long i = 0; i < rounds; i++) {
			dummyResult += pinvokeTest_doubleReturn_direct();
		}
		return rounds;
	}

	public long test_return_double_ptr() {
		long rounds = defaultRounds;
		for (long i = 0; i < rounds; i++) {
			double v;
			pinvokeTest_doubleReturn_ptr(&v);
			dummyResult += v;
		}
		return rounds;
	}

	public long test_callback_given_static() {
		long rounds = defaultRounds;
		for (long i = 0; i < rounds; i++) {
			dummyResult += pinvokeTest_invokeGivenStaticCallback(testStaticDelegatePtr, 1);
		}
		return rounds;
	}

	public long test_callback_stored_static() {
		long rounds = defaultRounds;
		pinvokeTest_setStaticCallback(testStaticDelegatePtr);
		for (long i = 0; i < rounds; i++) {
			dummyResult += pinvokeTest_invokeStoredStaticCallback(1);
		}
		return rounds;
	}

	public long test_callback_given_dynamic() {
		long rounds = defaultRounds;
		for (long i = 0; i < rounds; i++) {
			TestStaticDelegate t = k => k + 10;
			GCHandle handle = GCHandle.Alloc(t);
			dummyResult += pinvokeTest_invokeGivenDynamicCallback(testDynamicDelegatePtr, GCHandle.ToIntPtr(handle), 1);
			handle.Free();
		}
		return rounds;
	}

	public long test_callback_stored_dynamic() {
		long rounds = defaultRounds;
		TestStaticDelegate t = k => k;
		GCHandle handle = GCHandle.Alloc(t);
		pinvokeTest_setDynamicCallback(testDynamicDelegatePtr);
		for (long i = 0; i < rounds; i++) {
			dummyResult += pinvokeTest_invokeStoredDynamicCallback(GCHandle.ToIntPtr(handle), 1);
		}
		handle.Free();
		return rounds;
	}

	#if !ENABLE_IL2CPP
	public long test_callback_stored_dynamic_direct() {
		long rounds = defaultRounds;
		TestDynamicDelegate t = (obj, k) => k;
		GCHandle handle = GCHandle.Alloc(t);
		pinvokeTest_setDynamicCallback(Marshal.GetFunctionPointerForDelegate(t));
		for (long i = 0; i < rounds; i++) {
			dummyResult += pinvokeTest_invokeStoredDynamicCallback(GCHandle.ToIntPtr(handle), 1);
		}
		handle.Free();
		return rounds;
	}
	#endif

	public static List<PerformanceTest> getTests() {
		return PerformanceTests.getTestsFromClassByReflection(new PInvokeTests()).Select(test => {
			test.category = $"PInvoke tests";
			return test;
		}).ToList();
	}

}

