#pragma once

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Pass-by-value structs for which the C# definition is automatically generated from the C++ definition.

FXP struct TestStruct1 {
	int i1;
	unsigned char c1;
	unsigned char c2;

	// Should be aligned by 4 after the preceding 1-byte variable.
	int i2;

	// Mapped to a C# struct as defined in the config file.
	float4 v1;
	
	float4 *ptr1;
	
	struct IgnoredNestedStruct {
		int a1;
	};
	static int ignoredStaticVariable;
	void ignoredFunction();

	int array1[25];
	float4 array2[25]; // Arrays with non-primitive type are not supported by the C# language and are represented as byte arrays instead.

	int i4 = 4, i5[4], *ptr2, **ptr3; // Multiple variables on the same line, with array and pointer types.
	unsigned char c3;
};

FXP struct TestStruct2 {
	int i;

	// Structs can be nested normally.
	TestStruct1 s;
};

// In C# alignment has to be ensured manually.
FXP struct alignas(16) AlignedStruct1 {
	int test;
	__m128 v2;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Enums.

// An enum that is automatically replicated to C#.
FXP enum class Enum1 {
	// Default value.
	VALUE1,
	// Explicitly defined value.
	VALUE2 = 4,
	// Value by expression.
	VALUE3 = VALUE1 + VALUE2 + 1
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HLSL/GLSL structs, enums and constant buffers (also available in C#).

// An enum is converted to const variables in HLSL/GLSL.
FXPP(hlsl(prefix(test_)), glsl(prefix(test_))) enum class GpuEnum {
	VALUE1 = 1,
	VALUE2 = 2,
	VALUE3 = 3,

	// This comment should get copied
	// to the generated file.
	VALUE4 = 4
};

// Struct fields must be aligned in C++ according to the HLSL/GLSL packing rules. E.g. in HLSL, each element crossing a 16-byte boundary must be aligned by 16 bytes.
FXPP(hlsl, glsl) struct GpuStruct {
	// These two fields fill the first 16 bytes.
	float2 v1;
	float2 v2;

	// The next two fields fill the next 16 bytes. If these were declared in reverse order, GLSL would align the
	// 3-component vector to the next 16-byte boundary while HLSL would place it directly after the integer.
	float3 v3;
	int i3;

	int i4;
	//float4 v4; // Error because would cross a 16-byte boundary and would be packed differently in C++ and HLSL/GLSL.

	// Use the "alignas" keyword to align the vector the same way in C++ as in HLSL/GLSL. Another option is to add padding fields.
	alignas(16) float4 v4;

	float4 array1[2];
	//float3 array2[2]; // Error because HLSL/GLSL would pad the array elements to 16 bytes but C++ would not.
};

// An optional prefix can be added to all constant buffer / uniform block fields in case they are seen as global variables in HLSL/GLSL code.
// In GLSL, you can use the instanceName attribute to make the fields non-global.
// The constant buffer / uniform block can be (optionally) mapped to a specific HLSL register or a GLSL binding index.
// Variable initializer expressions are allowed, but won't be exported to C# (but you can call a C++ function that invokes a constructor to run the initializers).
FXPP(hlsl(cbuffer, prefix(g_), register(b0)), glsl(storage(uniform), prefix(g_), instanceName(constants))) struct alignas(16) GpuConstantBuffer {
	float4x4 m;
	float2 v1 = float2(1, 2);
	float v2 = 0;
	// This vector will be aligned automatically.
	__m128 v3;

	GpuStruct s;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Pass-by-reference classes.

FXP class TestClass {

	std::string name;

	typedef int IgnoredTypedef;

public:
	explicit TestClass(const std::string &name) : name(name) {
	}

	// Allows destroying the object from C#.
	FXP void release() {
		delete this;
	}

	// Static function.
	FXP static TestClass * create(std::string name) {
		return new TestClass(name);
	}

	FXP std::string getName() {
		return name;
	}

	// This function is ignored, because there's no FXP keyword, and it can use parameter types that are not mapped to C# types.
	void ignoredFunction(time_t time) {
	}

	FXP void setName(std::string const &name) {
		this->name = name;
	}
	
	FXP std::string concatenateStrings(std::string const &s1, std::string s2) {
		return s1 + s2;
	}

	FXP std::u16string concatenateStringsUtf16(std::u16string const &s1, std::u16string s2) {
		return s1 + s2;
	}

	// Exceptions are propagated to C#.
	FXP void throwException() {
		throw std::runtime_error("test_exception");
	}

	// A function using custom C++ vector types mapped to built-in C# types.
	FXP float4 addFloatVectors(const float4 &v1, const float4 &v2) {
		float4 result;
		_mm_storeu_ps((float *)&result, _mm_add_ps(_mm_loadu_ps((const float *)&v1), _mm_loadu_ps((const float *)&v2)));
		return result;
	}

	// A function using custom C++ vector types mapped to built-in C# types, with exceptions disabled.
	FXP float4 addFloatVectorsNoexcept(const float4 &v1, const float4 &v2) noexcept {
		float4 result;
		_mm_storeu_ps((float *)&result, _mm_add_ps(_mm_loadu_ps((const float *)&v1), _mm_loadu_ps((const float *)&v2)));
		return result;
	}

	// A high-performance SSE function that would return a color by using the two lowest elements of the given vector as 2d coordinates.
	// Note the vectorcall calling convention.
	__m128 VECTORCALL getColorXm(__m128 coords2d) {
		return _mm_add_ps(_mm_shuffle_ps(coords2d, coords2d, _MM_SHUFFLE(1, 0, 1, 0)), __m128{1, 2, 3, 4});
	}

	// A wrapper for the previous SSE function that does not require aligned parameters and takes a float2 parameter instead of float4 for the 2d coordinates.
	FXP float4 getColor(const float2 &coords) {
		float4 ret;
		_mm_storeu_ps((float *)&ret, getColorXm(coords.toM128()));
		return ret;
	}

	/**
	 * Comments are copied to C#.
	 */
	FXP void commentedFunction1() {
	}

	// Multiple consecutive single-line
	// comments like this are copied to C#.
	FXP void commentedFunction2() {
	}

	friend class IgnoredClass;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Derived classes.

FXP class BaseClass1 {
public:
	virtual ~BaseClass1() {}
	FXP virtual void release() { delete this; }
	FXP virtual std::string test1() { return "base1"; }
};

FXP class BaseClass2 {
public:
	virtual ~BaseClass2() {}
	FXP virtual void release() { delete this; }
	FXP virtual std::string test2() { return "base2"; }
};

FXP class BaseClass3 {
public:
	virtual ~BaseClass3() {}
	FXP virtual void release() { delete this; }
	FXP virtual std::string test3() { return "base3"; }
};

// Functions from public base classes will be accessible from C#, but those from private base classes won't.
FXP class DerivedClass : public BaseClass1, private BaseClass2, public BaseClass3 {
public:
	virtual ~DerivedClass() {}
	FXP void release() override { delete this; }

	FXP std::string test1() override { return "derived1"; }
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// A callback function received from C# can be stored as an std::function object that will prevent the C# object from being garbage-collected
// before the std::function and all its copies have been destroyed (with standard P/Invoke you would have to keep a C# reference manually,
// even if the callback is used only during the same function call).

FXP class CallbackTest {
	std::function<std::string(const std::string &, const std::string &)> storedCallback;
	
public:
	FXP void release() { delete this; }

	FXP std::string invokeGivenCallback(const std::string &s1, const std::string &s2, const std::function<std::string(const std::string &p1, const std::string &p2)> &callback) {
		return callback(s1, s2);
	}
	
	FXP std::u16string invokeGivenCallbackUtf16(const std::u16string &s1, const std::u16string &s2, const std::function<std::u16string(const std::u16string &p1, const std::u16string &p2)> &callback) {
		return callback(s1, s2);
	}
	
	FXP void setCallback(const std::function<std::string(const std::string &s1, const std::string &s2)> &callback) {
		storedCallback = callback;
	}

	FXP std::string invokeStoredCallback(const std::string &s1, const std::string &s2) {
		return storedCallback(s1, s2);
	}

	FXP int64_t invokeIntCallbackRepeatedly(int64_t rounds, const std::function<int(int i1, int i2)> &callback) {
		int64_t result = 0;
		for (int64_t i = 0; i < rounds; i++) {
			result += callback(1, 2);
		}
		return result;
	}

	FXP int64_t invokeStringParamCallbackRepeatedly(int64_t rounds, const std::function<int(std::string s)> &callback) {
		int64_t result = 0;
		std::string s("abc");
		for (int64_t i = 0; i < rounds; i++) {
			result += callback(s);
		}
		return result;
	}

	FXP int64_t invokeStringReturnValueCallbackRepeatedly(int64_t rounds, const std::function<std::string()> &callback) {
		int64_t result = 0;
		for (int64_t i = 0; i < rounds; i++) {
			result += callback().length();
		}
		return result;
	}
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Namespaces.

namespace CppOuterNamespace {

	FXP class TestClass2 {
		std::string name;
		int index;

	public:
		explicit TestClass2(const std::string &name, int index) : name(name), index(index) {
		}

		FXP void release() {
			delete this;
		}

		FXP std::string getName() {
			return name;
		}

		FXP int getIndex() {
			return index;
		}
	};

	FXP enum class EnumInsideNamespace {
		TEST1 = 1, TEST2 = 2
	};

	FXP int calculateSum(int v1, EnumInsideNamespace v2);
	
	namespace CppInnerNamespace {

		FXP struct StructInsideNamespace {
			int v;
		};
		
		FXP class TestClass3 {
			std::string name;

		public:
			explicit TestClass3(const std::string &name) : name(name) {
			}

			FXP void release() {
				delete this;
			}

			FXP std::string getName() {
				return name;
			}
		};

		FXP int calculateProduct(int v1, StructInsideNamespace v2);
	}

}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Global functions.

FXP CppOuterNamespace::TestClass2 * createTestClass2Instance(std::string name, int index);
FXP CppOuterNamespace::CppInnerNamespace::TestClass3 * createTestClass3Instance(std::string name);
FXP BaseClass1 * createBaseClass1Instance();
FXP BaseClass2 * createBaseClass2Instance();
FXP BaseClass3 * createBaseClass3Instance();
FXP DerivedClass * createDerivedClassInstance();
FXP CallbackTest * createCallbackTestInstance();

FXP void setStruct1Values(TestStruct1 *s);
FXP void setStruct2Values(TestStruct2 *s);
FXP void setCustomSharedStructValues(CustomSharedStruct *s);

FXP void emptyFunction() noexcept;

FXP std::string testEnumReflection();

FXP int64_t getTimeRdtsc() noexcept;

