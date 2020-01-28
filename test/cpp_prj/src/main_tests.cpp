#include "pch.h"
#include "main_tests.h"

#include "rabbitcall/rabbitcall_generated_partition1.h"


int CppOuterNamespace::calculateSum(int v1, EnumInsideNamespace v2) {
	return v1 + (int)v2;
}

int CppOuterNamespace::CppInnerNamespace::calculateProduct(int v1, StructInsideNamespace v2) {
	return v1 * v2.v;
}

CppOuterNamespace::TestClass2 * createTestClass2Instance(std::string name, int index) {
	return new CppOuterNamespace::TestClass2(name, index);
}

CppOuterNamespace::CppInnerNamespace::TestClass3 * createTestClass3Instance(std::string name) {
	return new CppOuterNamespace::CppInnerNamespace::TestClass3(name);
}

BaseClass1 * createBaseClass1Instance() {
	return new BaseClass1();
}

BaseClass2 * createBaseClass2Instance() {
	return new BaseClass2();
}

BaseClass3 * createBaseClass3Instance() {
	return new BaseClass3();
}

DerivedClass * createDerivedClassInstance() {
	return new DerivedClass();
}

CallbackTest * createCallbackTestInstance() {
	return new CallbackTest();
}

void setStruct1Values(TestStruct1 *s) {
	s->i1 = 100;
	s->v1 = float4(1, 2, 3, 4);
	s->array1[1] = 200;
}

void setStruct2Values(TestStruct2 *s) {
	s->i = 100;
	s->s.i1 = 200;
}

void setCustomSharedStructValues(CustomSharedStruct *s) {
	s->i1 = 700;
	s->v1 = float4(10, 20, 30, 40);
}

void emptyFunction() noexcept {
}

#define TEST_ENUM_REFLECTION_SHIFT_OPERATOR(p) \
	{\
		std::stringstream s;\
		s << Enum1::p;\
		if (s.str() != #p) return std::string("Enum operator<< returned wrong value, expected " #p ", got: ") + s.str();\
	}

#define TEST_ENUM_REFLECTION_TO_STRING(p) \
	{\
		std::string s = toString_Enum1(Enum1::p);\
		if (s != #p) return std::string("Enum toString returned wrong value, expected " #p ", got: ") + s;\
	}

#define TEST_ENUM_REFLECTION_PARSE(p) \
	{\
		Enum1 v = parse_Enum1(#p);\
		if (v != Enum1::p) return std::string("Enum parse returned wrong value, expected ") + std::to_string((int)Enum1::p) + ", got: " + std::to_string((int)v);\
	}

std::string testEnumReflection() {
	TEST_ENUM_REFLECTION_SHIFT_OPERATOR(VALUE1);
	TEST_ENUM_REFLECTION_SHIFT_OPERATOR(VALUE2);
	TEST_ENUM_REFLECTION_SHIFT_OPERATOR(VALUE3);
	TEST_ENUM_REFLECTION_TO_STRING(VALUE1);
	TEST_ENUM_REFLECTION_TO_STRING(VALUE2);
	TEST_ENUM_REFLECTION_TO_STRING(VALUE3);
	TEST_ENUM_REFLECTION_PARSE(VALUE1);
	TEST_ENUM_REFLECTION_PARSE(VALUE2);
	TEST_ENUM_REFLECTION_PARSE(VALUE3);
	return "";
}

int64_t getTimeRdtsc() noexcept {
	return (int64_t)__rdtsc();
}

