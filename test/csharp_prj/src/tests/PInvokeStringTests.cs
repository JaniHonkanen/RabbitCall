using System;
using System.Collections.Generic;
using System.Linq;
using System.Numerics;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Security;
using System.Text;
using CsNamespace;

public unsafe class PInvokeStringTests {

	long defaultRounds = 1000;
	string testString;
	public double dummyResult = 0;

	static Encoding latin1Encoding = Encoding.GetEncoding("ISO-8859-1");
	[ThreadStatic] static byte[] threadStaticBuffer;

	[SuppressUnmanagedCodeSecurity, DllImport(RabbitCallApi.cppLibraryFile, EntryPoint = "pinvokeTest_stringParam_bytePtr")]
	static extern void pinvokeTest_stringParam_byteLPStr([MarshalAs(UnmanagedType.LPStr)] string s);

	[SuppressUnmanagedCodeSecurity, DllImport(RabbitCallApi.cppLibraryFile, EntryPoint = "pinvokeTest_stringParam_bytePtr")]
	static extern void pinvokeTest_stringParam_LPUTF8Str([MarshalAs(UnmanagedType.LPUTF8Str)] string s);

	[SuppressUnmanagedCodeSecurity, DllImport(RabbitCallApi.cppLibraryFile, EntryPoint = "pinvokeTest_stringParam_bytePtr_createStdString")]
	static extern void pinvokeTest_stringParam_LPUTF8Str_createStdString([MarshalAs(UnmanagedType.LPUTF8Str)] string s);

	[SuppressUnmanagedCodeSecurity, DllImport(RabbitCallApi.cppLibraryFile, EntryPoint = "pinvokeTest_stringParam_bytePtr")]
	static extern void pinvokeTest_stringParam_bytePtr(void *p);

	[SuppressUnmanagedCodeSecurity, DllImport(RabbitCallApi.cppLibraryFile, EntryPoint = "pinvokeTest_stringParam_utf16Ptr")]
	static extern void pinvokeTest_stringParam_utf16LPWStr([MarshalAs(UnmanagedType.LPWStr)] string s);

	[SuppressUnmanagedCodeSecurity, DllImport(RabbitCallApi.cppLibraryFile, EntryPoint = "pinvokeTest_stringParam_utf16Ptr")]
	static extern void pinvokeTest_stringParam_utf16Ptr(void *p);

	[SuppressUnmanagedCodeSecurity, DllImport(RabbitCallApi.cppLibraryFile, EntryPoint = "pinvokeTest_stringParam_utf16PtrAndLength_createStdU16String")]
	static extern void pinvokeTest_stringParamUtf16PtrAndLength_createStdU16String(void *p, int length);

	[SuppressUnmanagedCodeSecurity, DllImport(RabbitCallApi.cppLibraryFile, EntryPoint = "pinvokeTest_stringParam_utf16PtrAndLength_createStdString")]
	static extern void pinvokeTest_stringParamUtf16PtrAndLength_createStdString(void *p, int length);

	[SuppressUnmanagedCodeSecurity, DllImport(RabbitCallApi.cppLibraryFile, EntryPoint = "pinvokeTest_setTestString")]
	static extern void pinvokeTest_setTestString([MarshalAs(UnmanagedType.LPStr)] string s);

	[SuppressUnmanagedCodeSecurity, DllImport(RabbitCallApi.cppLibraryFile, EntryPoint = "pinvokeTest_freeBuffer")]
	static extern void pinvokeTest_freeBuffer(void *p);

	[SuppressUnmanagedCodeSecurity, DllImport(RabbitCallApi.cppLibraryFile, EntryPoint = "pinvokeTest_stringReturn_coTaskMemAsciiString")]
	[return: MarshalAs(UnmanagedType.LPStr)]
	static extern string pinvokeTest_stringReturn_LPStr();

	[SuppressUnmanagedCodeSecurity, DllImport(RabbitCallApi.cppLibraryFile, EntryPoint = "pinvokeTest_stringReturn_coTaskMemAsciiString")]
	[return: MarshalAs(UnmanagedType.LPWStr)]
	static extern string pinvokeTest_stringReturn_LPWStr();

	[SuppressUnmanagedCodeSecurity, DllImport(RabbitCallApi.cppLibraryFile, EntryPoint = "pinvokeTest_stringReturn_bytePtr")]
	static extern void pinvokeTest_stringReturn_bytePtr(void **p, int *length);

	[SuppressUnmanagedCodeSecurity, DllImport(RabbitCallApi.cppLibraryFile, EntryPoint = "pinvokeTest_stringReturn_utf16Ptr")]
	static extern void pinvokeTest_stringReturn_utf16Ptr(void **p, int *length);

	[SuppressUnmanagedCodeSecurity, DllImport(RabbitCallApi.cppLibraryFile, EntryPoint = "pinvokeTest_stringReturn_reuseBufferUtf16")]
	static extern void pinvokeTest_stringReturn_reuseBufferUtf16(byte *buffer, int bufferSize, void **p, int *length);

	[SuppressUnmanagedCodeSecurity, DllImport(RabbitCallApi.cppLibraryFile, EntryPoint = "pinvokeTest_stringReturn_allocateBufferUtf16")]
	static extern void pinvokeTest_stringReturn_allocateBufferUtf16(void **p, int *length);

	public PInvokeStringTests(string testString) {
		this.testString = testString;
	}

	byte[] getOrCreateThreadStaticBuffer() {
		byte[] buffer = threadStaticBuffer;
		if (buffer == null) {
			buffer = new byte[16 * 1024];
			threadStaticBuffer = buffer;
		}
		return buffer;
	}

	public long test_param_string_LPWStr() {
		long rounds = defaultRounds;
		for (long i = 0; i < rounds; i++) {
			pinvokeTest_stringParam_utf16LPWStr(testString);
		}
		return rounds;
	}

	public long test_param_string_LPStr() {
		long rounds = defaultRounds;
		for (long i = 0; i < rounds; i++) {
			pinvokeTest_stringParam_byteLPStr(testString);
		}
		return rounds;
	}

	public long test_param_string_LPUTF8Str() {
		long rounds = defaultRounds;
		for (long i = 0; i < rounds; i++) {
			pinvokeTest_stringParam_LPUTF8Str(testString);
		}
		return rounds;
	}

	public long test_param_string_LPUTF8Str_createStdString() {
		long rounds = defaultRounds;
		for (long i = 0; i < rounds; i++) {
			pinvokeTest_stringParam_LPUTF8Str_createStdString(testString);
		}
		return rounds;
	}

	public long test_param_string_fixedPtr() {
		long rounds = defaultRounds;
		for (long i = 0; i < rounds; i++) {
			fixed (char *p = testString) {
				pinvokeTest_stringParam_utf16Ptr(p);
			}
		}
		return rounds;
	}

	public long test_param_string_fixedPtr_createStdU16String() {
		long rounds = defaultRounds;
		for (long i = 0; i < rounds; i++) {
			fixed (char *p = testString) {
				pinvokeTest_stringParamUtf16PtrAndLength_createStdU16String(p, testString.Length);
			}
		}
		return rounds;
	}

	public long test_param_string_fixedPtr_createStdString() {
		long rounds = defaultRounds;
		for (long i = 0; i < rounds; i++) {
			fixed (char *p = testString) {
				pinvokeTest_stringParamUtf16PtrAndLength_createStdString(p, testString.Length);
			}
		}
		return rounds;
	}

	public long test_param_string_convertToAscii_cs_newArray() {
		long rounds = defaultRounds;
		for (long i = 0; i < rounds; i++) {
			fixed (byte* p = Encoding.ASCII.GetBytes(testString)) {
				pinvokeTest_stringParam_bytePtr(p);
			}
		}
		return rounds;
	}

	public long test_param_string_convertToAscii_cs_threadStaticBuffer() {
		long rounds = defaultRounds;
		for (long i = 0; i < rounds; i++) {
			byte[] buffer = getOrCreateThreadStaticBuffer();
			fixed (byte* temp = buffer) {
				fixed (char *p = testString) {
					Encoding.ASCII.GetBytes(p, testString.Length, temp, buffer.Length);
				}
				pinvokeTest_stringParam_bytePtr(temp);
			}
		}
		return rounds;
	}

	public long test_param_string_convertToUtf8_cs_newArray() {
		long rounds = defaultRounds;
		for (long i = 0; i < rounds; i++) {
			fixed (byte* p = Encoding.UTF8.GetBytes(testString)) {
				pinvokeTest_stringParam_bytePtr(p);
			}
		}
		return rounds;
	}

	public long test_param_string_convertToUtf8_cs_threadStaticBuffer() {
		long rounds = defaultRounds;
		for (long i = 0; i < rounds; i++) {
			byte[] buffer = getOrCreateThreadStaticBuffer();
			fixed (byte* temp = buffer) {
				fixed (char *p = testString) {
					Encoding.UTF8.GetBytes(p, testString.Length, temp, buffer.Length);
				}
				pinvokeTest_stringParam_bytePtr(temp);
			}
		}
		return rounds;
	}

	// This is a separate method because stackalloc keeps the memory allocated until the method returns.
	void invoke_stringParam_convertToUtf8_cs_stackalloc() {
		int numBytes = Encoding.UTF8.GetByteCount(testString);
		byte *p = stackalloc byte[numBytes];
		fixed (char *src = testString) {
			Encoding.UTF8.GetBytes(src, testString.Length, p, numBytes);
		}
		pinvokeTest_stringParam_bytePtr(p);
	}

	public long test_param_string_convertToUtf8_cs_stackalloc() {
		long rounds = defaultRounds;
		for (long i = 0; i < rounds; i++) {
			invoke_stringParam_convertToUtf8_cs_stackalloc();
		}
		return rounds;
	}

	public long test_return_string_LPStr() {
		pinvokeTest_setTestString(testString);
		long rounds = defaultRounds;
		for (long i = 0; i < rounds; i++) {
			dummyResult += pinvokeTest_stringReturn_LPStr().Length;
		}
		return rounds;
	}

	public long test_return_string_LPWStr() {
		pinvokeTest_setTestString(testString);
		long rounds = defaultRounds;
		for (long i = 0; i < rounds; i++) {
			dummyResult += pinvokeTest_stringReturn_LPWStr().Length;
		}
		return rounds;
	}

	public long test_return_string_asciiPtr() {
		pinvokeTest_setTestString(testString);
		long rounds = defaultRounds;
		for (long i = 0; i < rounds; i++) {
			void *p;
			int length;
			pinvokeTest_stringReturn_bytePtr(&p, &length);
			string s = latin1Encoding.GetString((byte *)p, length);
			dummyResult += s.Length;
		}
		return rounds;
	}

	public long test_return_string_utf16Ptr() {
		pinvokeTest_setTestString(testString);
		long rounds = defaultRounds;
		for (long i = 0; i < rounds; i++) {
			void *p;
			int length;
			pinvokeTest_stringReturn_utf16Ptr(&p, &length);
			string s = new string((char *)p, 0, length);
			dummyResult += s.Length;
		}
		return rounds;
	}

	public long test_return_string_reuseBufferUtf16() {
		pinvokeTest_setTestString(testString);
		long rounds = defaultRounds;
		for (long i = 0; i < rounds; i++) {
			byte[] buffer = getOrCreateThreadStaticBuffer();
			fixed (byte* temp = buffer) {
				void *p;
				int length;
				pinvokeTest_stringReturn_reuseBufferUtf16(temp, buffer.Length, &p, &length);
				string s = new string((char *)p, 0, length);
				if (p != temp) pinvokeTest_freeBuffer(p);
				dummyResult += s.Length;
			}
		}
		return rounds;
	}

	public long test_return_string_allocateBufferUtf16() {
		pinvokeTest_setTestString(testString);
		long rounds = defaultRounds;
		for (long i = 0; i < rounds; i++) {
			void *p;
			int length;
			pinvokeTest_stringReturn_allocateBufferUtf16(&p, &length);
			string s = new string((char *)p, 0, length);
			pinvokeTest_freeBuffer(p);
			dummyResult += s.Length;
		}
		return rounds;
	}

	public static List<PerformanceTest> getTests() {
		List<PerformanceTest> result = new List<PerformanceTest>();

		string testString = "abcde12345";

		void addTests() {
			PerformanceTests.getTestsFromClassByReflection(new PInvokeStringTests(testString)).ForEach(test => {
				test.category = $"String tests (length {testString.Length})";
				result.Add(test);
			});
		}

		addTests();
		testString = string.Concat(Enumerable.Repeat(testString, 100));
		addTests();

		return result;
	}

}

