
using System;
using System.IO;
using System.Numerics;
using System.Runtime.InteropServices;
using CsNamespace;
using static TestUtil;

public unsafe class FunctionalTests {

	public void run(string projectDir, bool openGlTestEnabled) {
		try {
			// Create a C++ object and invoke some functions.
			{
				NTestClass obj = NTestClass.create("test1234€Ü");
				checkEqual(obj.isNull(), false, "isNull() returned wrong value");
				checkEqual(obj.getName(), "test1234€Ü", "Wrong name returned by getter");
				checkEqual(obj.concatenateStrings("abc€Ü", "def€Ü"), "abc€Üdef€Ü", "Strings not concatenated correctly");
				checkEqual(obj.concatenateStringsUtf16("ghi€Ü", "jkl€Ü"), "ghi€Üjkl€Ü", "UTF-16 strings not concatenated correctly");

				// An exception can be propagated from C++ to C#.
				try {
					obj.throwException();
				}
				catch (Exception e) {
					if (!e.Message.Contains("test_exception")) throw new Exception("Got wrong exception from C++ function", e);
				}

				// Vector math.
				checkEqual(obj.addFloatVectors(new Vector4(1, 2, 3, 4), new Vector4(5, 6, 7, 8)), new Vector4(6, 8, 10, 12), "Float vectors summed incorrectly");
				checkEqual(obj.addFloatVectorsNoexcept(new Vector4(2, 3, 4, 5), new Vector4(6, 7, 8, 9)), new Vector4(8, 10, 12, 14), "Float vectors summed incorrectly (noexcept version)");
				checkEqual(obj.getColor(new Vector2(1, 2)), new Vector4(2, 4, 4, 6), "getColor() returned wrong value");

				// Destroy the object.
				obj.release();

				checkEqual(NGlobal.partition2Test(), 22, "Partition 2 test function returned wrong value");
			}

			// Subclasses.
			{
				NDerivedClass obj = NGlobal.createDerivedClassInstance();
				checkEqual(obj.test1(), "derived1", "Overridden function returned wrong value");
				checkEqual(obj.test3(), "base3", "Non-overridden function returned wrong value");
				checkEqual(typeof(NDerivedClass).GetMethod("test2"), null, "test2() function should not be accessible because the base class was derived as private");

				// A derived-class pointer can be converted to base-class pointer.
				NBaseClass1 basePtr = obj;
				checkEqual(basePtr.test1(), "derived1", "Overridden function returned wrong value when invoked with a base class pointer");
				checkEqual(typeof(NBaseClass1).GetMethod("test3"), null, "Base class should not have functions that only exist in other base classes of the derived class");

				obj.release();
			}

			// Create a base class instance directly.
			{
				NBaseClass1 obj = NGlobal.createBaseClass1Instance();
				checkEqual(obj.test1(), "base1", "Function from base class instance returned wrong value");
				obj.release();
			}

			// Create a struct and access its fields by a pointer from both C++ and C#.
			{
				TestStruct1 s = new TestStruct1();
				string name = nameof(TestStruct1);
				checkEqual(s.i1, 0, $"{name} field i1 has wrong value before initialization");
				NGlobal.setStruct1Values(&s);
				checkEqual(s.i1, 100, $"{name} field i1 has wrong value");
				checkEqual(s.v1, new Vector4(1, 2, 3, 4), $"{name} field v1 has wrong value");
				checkEqual(s.array1[1], 200, $"{name} field array1[1] has wrong value");
			}
			{
				TestStruct2 s = new TestStruct2();
				string name = nameof(TestStruct2);
				checkEqual(s.i, 0, $"{name} field i has wrong value before initialization");
				NGlobal.setStruct2Values(&s);
				checkEqual(s.i, 100, $"{name} field s.i has wrong value");
			}
			{
				CustomSharedStruct s = new CustomSharedStruct();
				string name = nameof(CustomSharedStruct);
				checkEqual(s.i1, 0, $"{name} field i1 has wrong value before initialization");
				NGlobal.setCustomSharedStructValues(&s);
				checkEqual(s.i1, 700, $"{name} field i1 has wrong value");
				checkEqual(s.v1, new Vector4(10, 20, 30, 40), "Struct field v1 has wrong value");
			}

			// Callback functions
			{
				NCallbackTest cbTest = NGlobal.createCallbackTestInstance();

				checkEqual(cbTest.invokeGivenCallback("string5€Ü", "string6€Ü", (s1, s2) => s1 + s2), "string5€Üstring6€Ü", "C++ -> C# callback (given as parameter) should have concatenated strings");
				checkEqual(cbTest.invokeGivenCallbackUtf16("string7€Ü", "string8€Ü", (s1, s2) => s1 + s2), "string7€Üstring8€Ü", "C++ -> C# callback (given as parameter) should have concatenated UTF-16 strings");

				cbTest.setCallback((s1, s2) => s1 + s2);
				checkEqual(cbTest.invokeStoredCallback("string1€Ü", "string2€Ü"), "string1€Üstring2€Ü", "C++ -> C# callback (stored) should have concatenated strings");
			}

			// Namespaces.
			{
				CsNamespace.CppOuterNamespace.NTestClass2 obj = NGlobal.createTestClass2Instance("test5678", 123);
				checkEqual(obj.getName(), "test5678", "Wrong name returned by getter");
				checkEqual(obj.getIndex(), 123, "Wrong index returned by getter");
				obj.release();

				checkEqual(CsNamespace.CppOuterNamespace.NGlobal.calculateSum(1, CsNamespace.CppOuterNamespace.EnumInsideNamespace.TEST2), 3, "Wrong result from calculateSum()");
			}
			{
				CsNamespace.CppOuterNamespace.CppInnerNamespace.NTestClass3 obj = NGlobal.createTestClass3Instance("test_abc");
				checkEqual(obj.getName(), "test_abc", "Wrong name returned by getter");
				obj.release();

				CsNamespace.CppOuterNamespace.CppInnerNamespace.StructInsideNamespace s = new CsNamespace.CppOuterNamespace.CppInnerNamespace.StructInsideNamespace();
				s.v = 7;
				checkEqual(CsNamespace.CppOuterNamespace.CppInnerNamespace.NGlobal.calculateProduct(5, s), 35, "Wrong result from calculateProduct()");
			}

			// File set.
			{
				NGlobal.handleBicycle(new IncludedBicycleStruct());
				NGlobal.handleVehicle(new IncludedVehicleStruct());
				checkEqual(Type.GetType("CsNamespace.IncludedBicycleStruct") == null, false, "Struct from included file should be available");
				checkEqual(Type.GetType("CsNamespace.ExcludedCarStruct") == null, true, "Struct from excluded file not exist");
				checkEqual(Type.GetType("CsNamespace.ExcludedFileStruct") == null, true, "Struct from excluded file not exist");
			}

			// Enum reflection in C++.
			{
				string result = NGlobal.testEnumReflection();
				if (result.Length > 0) throw new Exception($"Error in enum reflection test: {result}");
			}

			// OpenGL.
			if (openGlTestEnabled) {
				NGlobal.testOpenGl(projectDir);
			}
			else {
				Log.write("Skipping OpenGL test", null, ConsoleColor.Yellow);
			}

			Log.write("*** FUNCTIONAL TEST SUCCESS ***", null, ConsoleColor.Green);
		}
		catch (Exception e) {
			Log.write("*** FUNCTIONAL TEST FAILED ***", e, ConsoleColor.Red);
		}

	}

}
