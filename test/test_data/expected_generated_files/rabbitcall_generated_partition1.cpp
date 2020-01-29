// This file was auto-generated by RabbitCall - do not modify manually.


#include "pch.h"

#include "../file_set_test/vehicle/bicycle/included_bicycle.h"
#include "../file_set_test/vehicle/included_vehicle.h"
#include "../main_tests.h"
#include "../opengl/opengl_test.h"
#include "../special_tests.h"

#include "rabbitcall_generated_main.h"

namespace RabbitCallInternalNamespace {
	
	RabbitCallEnum enum_Enum1;
	RabbitCallEnum enum_GpuEnum;
	RabbitCallEnum enum_EnumInsideNamespace;
	RabbitCallEnum enum_NonClassEnum;
}

using namespace RabbitCallInternalNamespace;

void RabbitCallInternalNamespace::initPartition_partition1() {
	rabbitCallInternal.typesByName["TestStruct1"] = new RabbitCallType("TestStruct1", sizeof(TestStruct1));
	rabbitCallInternal.typesByName["TestStruct2"] = new RabbitCallType("TestStruct2", sizeof(TestStruct2));
	rabbitCallInternal.typesByName["AlignedStruct1"] = new RabbitCallType("AlignedStruct1", sizeof(AlignedStruct1));
	rabbitCallInternal.typesByName["Enum1"] = new RabbitCallType("Enum1", 0);
	rabbitCallInternal.typesByName["GpuEnum"] = new RabbitCallType("GpuEnum", 0);
	rabbitCallInternal.typesByName["GpuStruct"] = new RabbitCallType("GpuStruct", sizeof(GpuStruct));
	rabbitCallInternal.typesByName["GpuConstantBuffer"] = new RabbitCallType("GpuConstantBuffer", sizeof(GpuConstantBuffer));
	rabbitCallInternal.typesByName["TestClass"] = new RabbitCallType("TestClass", 0);
	rabbitCallInternal.typesByName["BaseClass1"] = new RabbitCallType("BaseClass1", 0);
	rabbitCallInternal.typesByName["BaseClass2"] = new RabbitCallType("BaseClass2", 0);
	rabbitCallInternal.typesByName["BaseClass3"] = new RabbitCallType("BaseClass3", 0);
	rabbitCallInternal.typesByName["DerivedClass"] = new RabbitCallType("DerivedClass", 0);
	rabbitCallInternal.typesByName["CallbackTest"] = new RabbitCallType("CallbackTest", 0);
	rabbitCallInternal.typesByName["CppOuterNamespace::TestClass2"] = new RabbitCallType("CppOuterNamespace::TestClass2", 0);
	rabbitCallInternal.typesByName["CppOuterNamespace::EnumInsideNamespace"] = new RabbitCallType("CppOuterNamespace::EnumInsideNamespace", 0);
	rabbitCallInternal.typesByName["CppOuterNamespace::CppInnerNamespace::StructInsideNamespace"] = new RabbitCallType("CppOuterNamespace::CppInnerNamespace::StructInsideNamespace", sizeof(CppOuterNamespace::CppInnerNamespace::StructInsideNamespace));
	rabbitCallInternal.typesByName["CppOuterNamespace::CppInnerNamespace::TestClass3"] = new RabbitCallType("CppOuterNamespace::CppInnerNamespace::TestClass3", 0);
	rabbitCallInternal.typesByName["TestStruct3"] = new RabbitCallType("TestStruct3", sizeof(TestStruct3));
	rabbitCallInternal.typesByName["AnotherDerivedClass"] = new RabbitCallType("AnotherDerivedClass", 0);
	rabbitCallInternal.typesByName["NonClassEnum"] = new RabbitCallType("NonClassEnum", 0);
	rabbitCallInternal.typesByName["SpecialCasesStruct"] = new RabbitCallType("SpecialCasesStruct", sizeof(SpecialCasesStruct));
	rabbitCallInternal.typesByName["TestClassUsingNamespaceStd"] = new RabbitCallType("TestClassUsingNamespaceStd", 0);
	rabbitCallInternal.typesByName["IncludedVehicleStruct"] = new RabbitCallType("IncludedVehicleStruct", sizeof(IncludedVehicleStruct));
	rabbitCallInternal.typesByName["IncludedBicycleStruct"] = new RabbitCallType("IncludedBicycleStruct", sizeof(IncludedBicycleStruct));
	RabbitCallInternalNamespace::enum_Enum1.setMapping((int64_t)Enum1::VALUE3, "VALUE3");
	RabbitCallInternalNamespace::enum_Enum1.setMapping((int64_t)Enum1::VALUE2, "VALUE2");
	RabbitCallInternalNamespace::enum_Enum1.setMapping((int64_t)Enum1::VALUE1, "VALUE1");
	RabbitCallInternalNamespace::enum_GpuEnum.setMapping((int64_t)GpuEnum::VALUE4, "VALUE4");
	RabbitCallInternalNamespace::enum_GpuEnum.setMapping((int64_t)GpuEnum::VALUE3, "VALUE3");
	RabbitCallInternalNamespace::enum_GpuEnum.setMapping((int64_t)GpuEnum::VALUE2, "VALUE2");
	RabbitCallInternalNamespace::enum_GpuEnum.setMapping((int64_t)GpuEnum::VALUE1, "VALUE1");
	RabbitCallInternalNamespace::enum_EnumInsideNamespace.setMapping((int64_t)CppOuterNamespace::EnumInsideNamespace::TEST2, "TEST2");
	RabbitCallInternalNamespace::enum_EnumInsideNamespace.setMapping((int64_t)CppOuterNamespace::EnumInsideNamespace::TEST1, "TEST1");
	RabbitCallInternalNamespace::enum_NonClassEnum.setMapping((int64_t)NonClassEnum::VALUE3, "VALUE3");
	RabbitCallInternalNamespace::enum_NonClassEnum.setMapping((int64_t)NonClassEnum::VALUE2, "VALUE2");
	RabbitCallInternalNamespace::enum_NonClassEnum.setMapping((int64_t)NonClassEnum::VALUE1, "VALUE1");
}

Enum1 parse_Enum1(const std::string &s) { return (Enum1)RabbitCallInternalNamespace::enum_Enum1.parse(s); }
std::string toString_Enum1(Enum1 v) { return RabbitCallInternalNamespace::enum_Enum1.toString((int64_t)v); }
std::ostream & operator<<(std::ostream &os, const Enum1 &v) { return os << RabbitCallInternalNamespace::enum_Enum1.toString((int64_t)v); }

GpuEnum parse_GpuEnum(const std::string &s) { return (GpuEnum)RabbitCallInternalNamespace::enum_GpuEnum.parse(s); }
std::string toString_GpuEnum(GpuEnum v) { return RabbitCallInternalNamespace::enum_GpuEnum.toString((int64_t)v); }
std::ostream & operator<<(std::ostream &os, const GpuEnum &v) { return os << RabbitCallInternalNamespace::enum_GpuEnum.toString((int64_t)v); }

namespace CppOuterNamespace {
	EnumInsideNamespace parse_EnumInsideNamespace(const std::string &s) { return (EnumInsideNamespace)RabbitCallInternalNamespace::enum_EnumInsideNamespace.parse(s); }
	std::string toString_EnumInsideNamespace(EnumInsideNamespace v) { return RabbitCallInternalNamespace::enum_EnumInsideNamespace.toString((int64_t)v); }
	std::ostream & operator<<(std::ostream &os, const EnumInsideNamespace &v) { return os << RabbitCallInternalNamespace::enum_EnumInsideNamespace.toString((int64_t)v); }
}

NonClassEnum parse_NonClassEnum(const std::string &s) { return (NonClassEnum)RabbitCallInternalNamespace::enum_NonClassEnum.parse(s); }
std::string toString_NonClassEnum(NonClassEnum v) { return RabbitCallInternalNamespace::enum_NonClassEnum.toString((int64_t)v); }
std::ostream & operator<<(std::ostream &os, const NonClassEnum &v) { return os << RabbitCallInternalNamespace::enum_NonClassEnum.toString((int64_t)v); }

_RC_FUNC_EXC(rabbitcall_CppOuterNamespace_global_calculateSum(int a0,CppOuterNamespace::EnumInsideNamespace a1,int *_rc_r,_rc_PtrAndSize *_rc_e), *_rc_r = CppOuterNamespace::calculateSum(a0,a1);)
_RC_FUNC_EXC(rabbitcall_CppOuterNamespace_CppInnerNamespace_global_calculateProduct(int a0,CppOuterNamespace::CppInnerNamespace::StructInsideNamespace a1,int *_rc_r,_rc_PtrAndSize *_rc_e), *_rc_r = CppOuterNamespace::CppInnerNamespace::calculateProduct(a0,a1);)
_RC_FUNC_EXC(rabbitcall_global_createTestClass2Instance(const char *a0,int a1,CppOuterNamespace::TestClass2 **_rc_r,_rc_PtrAndSize *_rc_e), *_rc_r = createTestClass2Instance(std::string((const char *)a0),a1);)
_RC_FUNC_EXC(rabbitcall_global_createTestClass3Instance(const char *a0,CppOuterNamespace::CppInnerNamespace::TestClass3 **_rc_r,_rc_PtrAndSize *_rc_e), *_rc_r = createTestClass3Instance(std::string((const char *)a0));)
_RC_FUNC_EXC(rabbitcall_global_createBaseClass1Instance(BaseClass1 **_rc_r,_rc_PtrAndSize *_rc_e), *_rc_r = createBaseClass1Instance();)
_RC_FUNC_EXC(rabbitcall_global_createBaseClass2Instance(BaseClass2 **_rc_r,_rc_PtrAndSize *_rc_e), *_rc_r = createBaseClass2Instance();)
_RC_FUNC_EXC(rabbitcall_global_createBaseClass3Instance(BaseClass3 **_rc_r,_rc_PtrAndSize *_rc_e), *_rc_r = createBaseClass3Instance();)
_RC_FUNC_EXC(rabbitcall_global_createDerivedClassInstance(DerivedClass **_rc_r,_rc_PtrAndSize *_rc_e), *_rc_r = createDerivedClassInstance();)
_RC_FUNC_EXC(rabbitcall_global_createCallbackTestInstance(CallbackTest **_rc_r,_rc_PtrAndSize *_rc_e), *_rc_r = createCallbackTestInstance();)
_RC_FUNC_EXC(rabbitcall_global_setStruct1Values(TestStruct1 *a0,_rc_PtrAndSize *_rc_e), setStruct1Values(a0);)
_RC_FUNC_EXC(rabbitcall_global_setStruct2Values(TestStruct2 *a0,_rc_PtrAndSize *_rc_e), setStruct2Values(a0);)
_RC_FUNC_EXC(rabbitcall_global_setCustomSharedStructValues(CustomSharedStruct *a0,_rc_PtrAndSize *_rc_e), setCustomSharedStructValues(a0);)
_RC_FUNC_NOEXC(rabbitcall_global_emptyFunction(), emptyFunction();)
_RC_FUNC_EXC(rabbitcall_global_testEnumReflection(_rc_PtrAndSize *_rc_r,_rc_PtrAndSize *_rc_e), *_rc_r = _rc_createString(testEnumReflection());)
_RC_FUNC_NOEXC(rabbitcall_global_getTimeRdtsc(int64_t *_rc_r), *_rc_r = getTimeRdtsc();)
_RC_FUNC_EXC(rabbitcall_global_createAnotherDerivedClass(AnotherDerivedClass **_rc_r,_rc_PtrAndSize *_rc_e), *_rc_r = createAnotherDerivedClass();)
_RC_FUNC_EXC(rabbitcall_global_testParameterNameClash(int a0,int a1,int a2,int *_rc_r,_rc_PtrAndSize *_rc_e), *_rc_r = testParameterNameClash(a0,a1,a2);)
_RC_FUNC_EXC(rabbitcall_global_handleVehicle(IncludedVehicleStruct a0,_rc_PtrAndSize *_rc_e), handleVehicle(a0);)
_RC_FUNC_EXC(rabbitcall_global_handleBicycle(IncludedBicycleStruct a0,_rc_PtrAndSize *_rc_e), handleBicycle(a0);)
_RC_FUNC_EXC(rabbitcall_global_testOpenGl(_rc_PtrAndSize *_rc_e), testOpenGl();)

_RC_FUNC_EXC(rabbitcall_TestClass_release(TestClass *_rc_t,_rc_PtrAndSize *_rc_e), _rc_t->release();)
_RC_FUNC_EXC(rabbitcall_TestClass_create(const char *a0,TestClass **_rc_r,_rc_PtrAndSize *_rc_e), *_rc_r = TestClass::create(std::string((const char *)a0));)
_RC_FUNC_EXC(rabbitcall_TestClass_getName(TestClass *_rc_t,_rc_PtrAndSize *_rc_r,_rc_PtrAndSize *_rc_e), *_rc_r = _rc_createString(_rc_t->getName());)
_RC_FUNC_EXC(rabbitcall_TestClass_setName(TestClass *_rc_t,const char *a0,_rc_PtrAndSize *_rc_e), _rc_t->setName(std::string((const char *)a0));)
_RC_FUNC_EXC(rabbitcall_TestClass_concatenateStrings(TestClass *_rc_t,const char *a0,const char *a1,_rc_PtrAndSize *_rc_r,_rc_PtrAndSize *_rc_e), *_rc_r = _rc_createString(_rc_t->concatenateStrings(std::string((const char *)a0),std::string((const char *)a1)));)
_RC_FUNC_EXC(rabbitcall_TestClass_concatenateStringsUtf16(TestClass *_rc_t,const char16_t *a0,const char16_t *a1,_rc_PtrAndSize *_rc_r,_rc_PtrAndSize *_rc_e), *_rc_r = _rc_createString(_rc_t->concatenateStringsUtf16(std::u16string((const char16_t *)a0),std::u16string((const char16_t *)a1)));)
_RC_FUNC_EXC(rabbitcall_TestClass_throwException(TestClass *_rc_t,_rc_PtrAndSize *_rc_e), _rc_t->throwException();)
_RC_FUNC_EXC(rabbitcall_TestClass_addFloatVectors(TestClass *_rc_t,float4 a0,float4 a1,float4 *_rc_r,_rc_PtrAndSize *_rc_e), *_rc_r = _rc_t->addFloatVectors(a0,a1);)
_RC_FUNC_NOEXC(rabbitcall_TestClass_addFloatVectorsNoexcept(TestClass *_rc_t,float4 a0,float4 a1,float4 *_rc_r), *_rc_r = _rc_t->addFloatVectorsNoexcept(a0,a1);)
_RC_FUNC_EXC(rabbitcall_TestClass_getColor(TestClass *_rc_t,float2 a0,float4 *_rc_r,_rc_PtrAndSize *_rc_e), *_rc_r = _rc_t->getColor(a0);)
_RC_FUNC_EXC(rabbitcall_TestClass_commentedFunction1(TestClass *_rc_t,_rc_PtrAndSize *_rc_e), _rc_t->commentedFunction1();)
_RC_FUNC_EXC(rabbitcall_TestClass_commentedFunction2(TestClass *_rc_t,_rc_PtrAndSize *_rc_e), _rc_t->commentedFunction2();)

_RC_FUNC_EXC(rabbitcall_BaseClass1_release(BaseClass1 *_rc_t,_rc_PtrAndSize *_rc_e), _rc_t->release();)
_RC_FUNC_EXC(rabbitcall_BaseClass1_test1(BaseClass1 *_rc_t,_rc_PtrAndSize *_rc_r,_rc_PtrAndSize *_rc_e), *_rc_r = _rc_createString(_rc_t->test1());)

_RC_FUNC_EXC(rabbitcall_BaseClass2_release(BaseClass2 *_rc_t,_rc_PtrAndSize *_rc_e), _rc_t->release();)
_RC_FUNC_EXC(rabbitcall_BaseClass2_test2(BaseClass2 *_rc_t,_rc_PtrAndSize *_rc_r,_rc_PtrAndSize *_rc_e), *_rc_r = _rc_createString(_rc_t->test2());)

_RC_FUNC_EXC(rabbitcall_BaseClass3_release(BaseClass3 *_rc_t,_rc_PtrAndSize *_rc_e), _rc_t->release();)
_RC_FUNC_EXC(rabbitcall_BaseClass3_test3(BaseClass3 *_rc_t,_rc_PtrAndSize *_rc_r,_rc_PtrAndSize *_rc_e), *_rc_r = _rc_createString(_rc_t->test3());)

_RC_FUNC_EXC(rabbitcall_DerivedClass_release(DerivedClass *_rc_t,_rc_PtrAndSize *_rc_e), _rc_t->release();)
_RC_FUNC_EXC(rabbitcall_DerivedClass_test1(DerivedClass *_rc_t,_rc_PtrAndSize *_rc_r,_rc_PtrAndSize *_rc_e), *_rc_r = _rc_createString(_rc_t->test1());)
_RC_FUNC_EXC(rabbitcall_DerivedClass_test3(DerivedClass *_rc_t,_rc_PtrAndSize *_rc_r,_rc_PtrAndSize *_rc_e), *_rc_r = _rc_createString(_rc_t->test3());)

_RC_FUNC_EXC(rabbitcall_CallbackTest_release(CallbackTest *_rc_t,_rc_PtrAndSize *_rc_e), _rc_t->release();)
_RC_CALLBACK(_rc_Cb_CallbackTest_invokeGivenCallback_callback, typedef const char * (*FunctionPtrType)(const char *, const char *), std::string operator()(std::string p1, std::string p2) { const char *_rc_rt = cb->callbackFunction(p1.c_str(), p2.c_str()); std::string _rc_rt2(_rc_rt); _rc_deallocTaskMem((void *)_rc_rt); return _rc_rt2; })
_RC_FUNC_EXC(rabbitcall_CallbackTest_invokeGivenCallback(CallbackTest *_rc_t,const char *a0,const char *a1,const char * (*a2)(const char *, const char *),void *cb0,_rc_PtrAndSize *_rc_r,_rc_PtrAndSize *_rc_e), *_rc_r = _rc_createString(_rc_t->invokeGivenCallback(std::string((const char *)a0),std::string((const char *)a1),_rc_Cb_CallbackTest_invokeGivenCallback_callback(new _rc_CbH<const char * (*)(const char *, const char *)>(a2, cb0))));)
_RC_CALLBACK(_rc_Cb_CallbackTest_invokeGivenCallbackUtf16_callback, typedef const char16_t * (*FunctionPtrType)(const char16_t *, const char16_t *), std::u16string operator()(std::u16string p1, std::u16string p2) { const char16_t *_rc_rt = cb->callbackFunction(p1.c_str(), p2.c_str()); std::u16string _rc_rt2(_rc_rt); _rc_deallocTaskMem((void *)_rc_rt); return _rc_rt2; })
_RC_FUNC_EXC(rabbitcall_CallbackTest_invokeGivenCallbackUtf16(CallbackTest *_rc_t,const char16_t *a0,const char16_t *a1,const char16_t * (*a2)(const char16_t *, const char16_t *),void *cb0,_rc_PtrAndSize *_rc_r,_rc_PtrAndSize *_rc_e), *_rc_r = _rc_createString(_rc_t->invokeGivenCallbackUtf16(std::u16string((const char16_t *)a0),std::u16string((const char16_t *)a1),_rc_Cb_CallbackTest_invokeGivenCallbackUtf16_callback(new _rc_CbH<const char16_t * (*)(const char16_t *, const char16_t *)>(a2, cb0))));)
_RC_CALLBACK(_rc_Cb_CallbackTest_setCallback_callback, typedef const char * (*FunctionPtrType)(const char *, const char *), std::string operator()(std::string s1, std::string s2) { const char *_rc_rt = cb->callbackFunction(s1.c_str(), s2.c_str()); std::string _rc_rt2(_rc_rt); _rc_deallocTaskMem((void *)_rc_rt); return _rc_rt2; })
_RC_FUNC_EXC(rabbitcall_CallbackTest_setCallback(CallbackTest *_rc_t,const char * (*a0)(const char *, const char *),void *cb0,_rc_PtrAndSize *_rc_e), _rc_t->setCallback(_rc_Cb_CallbackTest_setCallback_callback(new _rc_CbH<const char * (*)(const char *, const char *)>(a0, cb0)));)
_RC_FUNC_EXC(rabbitcall_CallbackTest_invokeStoredCallback(CallbackTest *_rc_t,const char *a0,const char *a1,_rc_PtrAndSize *_rc_r,_rc_PtrAndSize *_rc_e), *_rc_r = _rc_createString(_rc_t->invokeStoredCallback(std::string((const char *)a0),std::string((const char *)a1)));)
_RC_CALLBACK(_rc_Cb_CallbackTest_invokeIntCallbackRepeatedly_callback, typedef int (*FunctionPtrType)(int, int), int operator()(int i1, int i2) { int _rc_rt = cb->callbackFunction(i1, i2); return _rc_rt; })
_RC_FUNC_EXC(rabbitcall_CallbackTest_invokeIntCallbackRepeatedly(CallbackTest *_rc_t,int64_t a0,int (*a1)(int, int),void *cb0,int64_t *_rc_r,_rc_PtrAndSize *_rc_e), *_rc_r = _rc_t->invokeIntCallbackRepeatedly(a0,_rc_Cb_CallbackTest_invokeIntCallbackRepeatedly_callback(new _rc_CbH<int (*)(int, int)>(a1, cb0)));)
_RC_CALLBACK(_rc_Cb_CallbackTest_invokeStringParamCallbackRepeatedly_callback, typedef int (*FunctionPtrType)(const char *), int operator()(std::string s) { int _rc_rt = cb->callbackFunction(s.c_str()); return _rc_rt; })
_RC_FUNC_EXC(rabbitcall_CallbackTest_invokeStringParamCallbackRepeatedly(CallbackTest *_rc_t,int64_t a0,int (*a1)(const char *),void *cb0,int64_t *_rc_r,_rc_PtrAndSize *_rc_e), *_rc_r = _rc_t->invokeStringParamCallbackRepeatedly(a0,_rc_Cb_CallbackTest_invokeStringParamCallbackRepeatedly_callback(new _rc_CbH<int (*)(const char *)>(a1, cb0)));)
_RC_CALLBACK(_rc_Cb_CallbackTest_invokeStringReturnValueCallbackRepeatedly_callback, typedef const char * (*FunctionPtrType)(), std::string operator()() { const char *_rc_rt = cb->callbackFunction(); std::string _rc_rt2(_rc_rt); _rc_deallocTaskMem((void *)_rc_rt); return _rc_rt2; })
_RC_FUNC_EXC(rabbitcall_CallbackTest_invokeStringReturnValueCallbackRepeatedly(CallbackTest *_rc_t,int64_t a0,const char * (*a1)(),void *cb0,int64_t *_rc_r,_rc_PtrAndSize *_rc_e), *_rc_r = _rc_t->invokeStringReturnValueCallbackRepeatedly(a0,_rc_Cb_CallbackTest_invokeStringReturnValueCallbackRepeatedly_callback(new _rc_CbH<const char * (*)()>(a1, cb0)));)

_RC_FUNC_EXC(rabbitcall_CppOuterNamespace_TestClass2_release(CppOuterNamespace::TestClass2 *_rc_t,_rc_PtrAndSize *_rc_e), _rc_t->release();)
_RC_FUNC_EXC(rabbitcall_CppOuterNamespace_TestClass2_getName(CppOuterNamespace::TestClass2 *_rc_t,_rc_PtrAndSize *_rc_r,_rc_PtrAndSize *_rc_e), *_rc_r = _rc_createString(_rc_t->getName());)
_RC_FUNC_EXC(rabbitcall_CppOuterNamespace_TestClass2_getIndex(CppOuterNamespace::TestClass2 *_rc_t,int *_rc_r,_rc_PtrAndSize *_rc_e), *_rc_r = _rc_t->getIndex();)

_RC_FUNC_EXC(rabbitcall_CppOuterNamespace_CppInnerNamespace_TestClass3_release(CppOuterNamespace::CppInnerNamespace::TestClass3 *_rc_t,_rc_PtrAndSize *_rc_e), _rc_t->release();)
_RC_FUNC_EXC(rabbitcall_CppOuterNamespace_CppInnerNamespace_TestClass3_getName(CppOuterNamespace::CppInnerNamespace::TestClass3 *_rc_t,_rc_PtrAndSize *_rc_r,_rc_PtrAndSize *_rc_e), *_rc_r = _rc_createString(_rc_t->getName());)

_RC_FUNC_EXC(rabbitcall_AnotherDerivedClass_release(AnotherDerivedClass *_rc_t,_rc_PtrAndSize *_rc_e), _rc_t->release();)
_RC_FUNC_EXC(rabbitcall_AnotherDerivedClass_test1(AnotherDerivedClass *_rc_t,_rc_PtrAndSize *_rc_r,_rc_PtrAndSize *_rc_e), *_rc_r = _rc_createString(_rc_t->test1());)
_RC_FUNC_EXC(rabbitcall_AnotherDerivedClass_test3(AnotherDerivedClass *_rc_t,_rc_PtrAndSize *_rc_r,_rc_PtrAndSize *_rc_e), *_rc_r = _rc_createString(_rc_t->test3());)

_RC_FUNC_EXC(rabbitcall_TestClassUsingNamespaceStd_concatenateStrings(TestClassUsingNamespaceStd *_rc_t,const char *a0,const char *a1,_rc_PtrAndSize *_rc_r,_rc_PtrAndSize *_rc_e), *_rc_r = _rc_createString(_rc_t->concatenateStrings(std::string((const char *)a0),std::string((const char *)a1)));)
