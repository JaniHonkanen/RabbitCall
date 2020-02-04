#include "pch.h"

#if defined (_MSC_VER)
#include <windows.h>
#else
// On Linux, PInvoke uses malloc/free instead of CoTaskMemAlloc/Free when returning strings.
#define CoTaskMemAlloc malloc
#define CoTaskMemFree free
#endif

#pragma warning(disable : 4190)
#pragma warning(disable : 4244)
#pragma warning(disable : 4297)

uint8_t pinvokeDummy[4096];
std::string pinvokeStringDummy;
std::u16string pinvokeU16StringDummy;
std::string testString;
std::u16string testU16String;
float4 testVector(1, 2, 3, 4);
int (*testStaticCallback)(int);
int (*testDynamicCallback)(void *, int);

extern "C" RC_EXPORT uint8_t * pinvokeTest_getDummyResult() noexcept {
	// Make sure the C++ compiler doesn't optimize away writing to the dummy variable.
	return pinvokeDummy + pinvokeStringDummy.length() + pinvokeU16StringDummy.length();
}

extern "C" RC_EXPORT void pinvokeTest_stringParam_bytePtr(const char *s) noexcept {
}

extern "C" RC_EXPORT void pinvokeTest_stringParam_bytePtr_createStdString(const char *s) noexcept {
	size_t length = strlen(s);
	std::string temp;
	temp.append(s, length);
	pinvokeStringDummy = temp;
}

extern "C" RC_EXPORT void pinvokeTest_stringParam_utf16Ptr(const char16_t *s) noexcept {
}

extern "C" RC_EXPORT void pinvokeTest_stringParam_utf16PtrAndLength_createStdU16String(const char16_t *s, int length) noexcept {
	std::u16string temp;
	temp.append(s, length);
	pinvokeU16StringDummy = temp;
}

extern "C" RC_EXPORT void pinvokeTest_stringParam_utf16PtrAndLength_createStdString(const char16_t *s, int length) noexcept {
	std::string temp;
	temp.resize(length);
	char *p = temp.data();
	for (int i = 0; i < length; i++) {
		p[i] = (char)s[i];
	}
	pinvokeStringDummy = temp;
}

extern "C" RC_EXPORT void pinvokeTest_setTestString(const char16_t *s) noexcept {
	testU16String = s;
	testString = std::string(testU16String.begin(), testU16String.end());
}

extern "C" RC_EXPORT void pinvokeTest_freeBuffer(void *s) noexcept {
	free(s);
}

extern "C" RC_EXPORT const char * pinvokeTest_stringReturn_coTaskMemAsciiString() noexcept {
	size_t lengthWithNullTerminator = testString.size() + 1;
	char *s = (char *)CoTaskMemAlloc(lengthWithNullTerminator);
	if (!s) {
		// Out-of-memory not properly handled in this test.
		return nullptr;
	}
	memcpy(s, testString.c_str(), lengthWithNullTerminator);
	return s;
}

extern "C" RC_EXPORT void pinvokeTest_stringReturn_bytePtr(void **ptrOut, int *lengthOut) noexcept {
	*ptrOut = testString.data();
	*lengthOut = (int)testString.size();
}

extern "C" RC_EXPORT void pinvokeTest_stringReturn_utf16Ptr(void **ptrOut, int *lengthOut) noexcept {
	*ptrOut = testU16String.data();
	*lengthOut = (int)testU16String.size();
}

extern "C" RC_EXPORT void pinvokeTest_stringReturn_reuseBufferUtf16(void *buffer, int bufferSize, void **ptrOut, int *lengthOut) noexcept {
	void *result;
	int64_t length = testU16String.size();
	if (length > bufferSize) {
		result = malloc(length * sizeof(char16_t));
		if (!result) {
			// Out-of-memory not properly handled in this test.
			*ptrOut = nullptr;
			*lengthOut = 0;
			return;
		}
	}
	else {
		result = buffer;
	}
	memcpy(result, testU16String.data(), testU16String.size() * sizeof(char16_t));
	*ptrOut = result;
	*lengthOut = (int)length;
}

extern "C" RC_EXPORT void pinvokeTest_stringReturn_allocateBufferUtf16(void **ptrOut, int *lengthOut) noexcept {
	int64_t length = testU16String.size();
	void *result = malloc(length * sizeof(char16_t));
	if (!result) {
		// Out-of-memory not properly handled in this test.
		*ptrOut = nullptr;
		*lengthOut = 0;
		return;
	}
	memcpy(result, testU16String.data(), testU16String.size() * sizeof(char16_t));
	*ptrOut = result;
	*lengthOut = (int)length;
}

extern "C" RC_EXPORT void pinvokeTest_vector4Param_direct(float4 v) noexcept {
}

extern "C" RC_EXPORT void pinvokeTest_vector4Param_ptr(float4 v) noexcept {
}

extern "C" RC_EXPORT void pinvokeTest_vector2Param_direct(float2 v) noexcept {
}

extern "C" RC_EXPORT void pinvokeTest_vector2Param_ptr(float2 v) noexcept {
}

extern "C" RC_EXPORT float4 pinvokeTest_vector4Return_direct() noexcept {
	return testVector;
}

extern "C" RC_EXPORT void pinvokeTest_vector4Return_ptr(float4 *v) noexcept {
	*v = testVector;
}

extern "C" RC_EXPORT double pinvokeTest_intReturn_direct() noexcept {
	return 1;
}

extern "C" RC_EXPORT void pinvokeTest_intReturn_ptr(double *v) noexcept {
	*v = 1;
}

extern "C" RC_EXPORT double pinvokeTest_doubleReturn_direct() noexcept {
	return 1.0;
}

extern "C" RC_EXPORT void pinvokeTest_doubleReturn_ptr(double *v) noexcept {
	*v = 1.0;
}

extern "C" RC_EXPORT int pinvokeTest_invokeGivenStaticCallback(int (*cb)(int), int v) noexcept {
	return cb(v);
}

extern "C" RC_EXPORT void pinvokeTest_setStaticCallback(int (*cb)(int)) noexcept {
	testStaticCallback = cb;
}

extern "C" RC_EXPORT int pinvokeTest_invokeStoredStaticCallback(int v) noexcept {
	return testStaticCallback(v);
}

extern "C" RC_EXPORT int pinvokeTest_invokeGivenDynamicCallback(int (*cb)(void *, int), void *obj, int v) noexcept {
	return cb(obj, v);
}

extern "C" RC_EXPORT void pinvokeTest_setDynamicCallback(int (*cb)(void *, int)) noexcept {
	testDynamicCallback = cb;
}

extern "C" RC_EXPORT int pinvokeTest_invokeStoredDynamicCallback(void *obj, int v) noexcept {
	return testDynamicCallback(obj, v);
}





