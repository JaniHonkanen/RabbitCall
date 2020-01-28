#pragma once
#include "main_tests.h"

// A struct that uses a struct from another file.
FXP struct TestStruct3 {
	int i = 1;
	unsigned char c = 'a';

	// Should be aligned.
	TestStruct1 s;
	
	TestStruct1 *ptr;

	signed long long i2 = 2;
	
	TestStruct3() {
		s = TestStruct1();
		ptr = nullptr;
	}

	~TestStruct3() {
		ptr = nullptr;
	}
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// A class that is derived from a class in another file.
FXP class AnotherDerivedClass : public DerivedClass {
public:
	/**
	 * This comment should appear in the generated file.
	 */
	[[nodiscard]] // This attribute and this comment should not appear in the function comment in the generated file.
	FXP std::string test1() override { return "anotherDerived"; }
};

FXP AnotherDerivedClass * createAnotherDerivedClass();

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

FXP enum NonClassEnum {
	// Default value.
	VALUE1,
	// Explicitly defined value.
	VALUE2 = 4,
	// Value by expression.
	VALUE3 = VALUE1 + VALUE2 + 1
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

FXP struct alignas( /* comment */ 16 ) SpecialCasesStruct {
	int i;
	int i2[ /* comment */ 4 /* comment */ ];

	// Check that these are identified as functions despite the equal sign in the operator that could make them look like variable assignments.
	SpecialCasesStruct &operator=(const SpecialCasesStruct &v) = default;
	SpecialCasesStruct &operator +=(const SpecialCasesStruct &v) {
		i += v.i;
		return *this;
	}
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Test parameter names that would clash with internal names used in the wrapper method (other names should be generated that don't clash with the parameters).
FXP int testParameterNameClash(int _rc_r, int _rc_e, int _rc_e_);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Test that the parser supports backslashes at the end of line.

// The tool should not try to parse the export keyword on the second line, because it belongs to the macro due to the backslash.
#define BACKSLASH_TEST test \
	FXP ;

inline void stringTest() {
	// Should not terminate parsing the string literal at the escaped quote or at the line-break (because of the backslash),
	// and therefore should not try to interpret the export keyword on the next line.
	std::string s1 = "sdljf\"skl \
		FXP ;";

	// Raw string literal where the string continues on the next line without a backslash.
	auto s2 = R"delim(sd flskskl s
	FXP ;
	sldfjsldkf
	jskfsdklf kl )delim";
	auto s3 = u8R"delim(sd flskskl s
	FXP ;
	sldfjsldkf
	jskfsdklf kl )delim";
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// "using namespace" directive. Names are accepted by the export tool also when the directive is in another
// header file which is not parsed, but ambiguous names must be qualified in exported declarations.

using namespace std;

FXP class TestClassUsingNamespaceStd {
public:
	// "std::" prefix not needed because of the using directive above.
	FXP string concatenateStrings(string s1, string s2) {
		return s1 + s2;
	}
};

