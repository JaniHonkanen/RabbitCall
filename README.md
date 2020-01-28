# RabbitCall

<img align="right" src="rabbit.png" style="padding: 10px;"/>This little helper allows you to mix C++ and C# in the same application without writing boilerplate code or repeating definitions in different languages. It generates binding code for calling functions between C++ and C#, and translates C++ structures and other elements to C# and HLSL/GLSL.

While other tools exist with a similar purpose, this one puts emphasis on easy/fast development and real-time performance (e.g. for games and graphics engines).

Benefits in comparison to other tools:

- Easy to use: just add keyword `FXP` in front of your C++ class/struct/function/enum declarations to make them visible to C#.
- Fast build speed: processes your C++ source code at a rate of ~100 MB/s => works well with large projects.
- Fast application development: make changes to the C++ interface, hit the compile button (which runs the tool as a quick pre-build task) and the updated C# interface will be immediately available in the C# IDE, even if the main C++ build is still running.
- Fast function call performance from C# to C++: 10 CPU cycles overhead for an empty function, 20 cycles for a function that returns the sum of two given `Vector4`'s.
- Zero C# GC overhead in most use cases, including C++ object creation and pointer wrapping in C# => suitable for real-time applications (wrappers implemented as C# structs instead of classes).
- Supports C++ exception propagation to C#.
- HLSL/GLSL support: C++ structs and enums can be exported to HLSL/GLSL as structs, constant buffers, uniform blocks and constant variables.
- Some C++ reflection features for enums, e.g. `operator<<`, `toString()`, `parse()`.
- Safer callbacks: C# delegates and lambda functions that are passed to C++ are wrapped in std::function objects that prevent the C# object from being garbage-collected until the std::function and all its copies are released (which is a cause of random crashes with some other C++/C# interop tools).
- Fewer compatibility issues from unrelated code: does not attempt to understand everything in your C++ codebase and included 3rd party headers, but concentrates on the C++/C# interface and features you most likely need.

Limitations:

- Currently supports x64 mode only.
- Some C++ features are not supported in the C++/C# interface (e.g. macros, typedefs, templates, standard container classes, move semantics, only partial namespace support). You can still use any C++ feature in your project as a whole, although not in functions that are exported to C#.
  - A workaround for typedefs is to add them as type mappings in the XML configuration file.
  - A workaround for other unsupported features is usually to create a wrapper function/interface around the unsupported C++ function/interface, using only supported features.
  - You can always use plain P/Invoke for some of your functions, even in generated classes (by using C# partial classes/structs or extension methods).

## Supported platforms

The tool has been built for x64 on Windows and Linux, but is likely to work on many other platforms if you compile the tool yourself.

As for C# environments, .Net Core, .Net Framework and Mono are all supported.

## Installation

Once you have a C++ library project (.dll or .so) and a C# project that is supposed to use the C++ library, take the following steps to install the tool (assuming you are using Visual Studio):

1. Include the `rabbitcall.h` file from the test project in your C++ project's headers.
2. Copy the configuration file from the test program ([rabbitcall.xml](test/rabbitcall.xml)) to e.g. your C++ project folder. See the instructions in the file and update configuration settings if necessary (set paths, remove unnecessary type mappings).
4. Enable unsafe code in the C# project settings (Build -> Allow unsafe code).
5. If you want the tool to run automatically when building, add it to the C++ project: Configuration Properties -> Build Events -> Pre-Build Event for both release/debug configurations and set the paths:
   `/path/to/rabbitcall.exe -configFile relative/path/to/rabbitcall.xml`
6. Run the tool (by running the build if you added it as a pre-build event).
7. Add the generated `rabbitcall_generated*.h/.cpp/.cs/.hlsl/.glsl` files into your projects as source files.
8. If you need C++ enum reflection, #include the auto-generated header files (`rabbitcall_generated*.h`) in your C++ source files (after all other #includes).
9. In the beginning of your C# program, invoke `RabbitCallApi.init()`.

The tool can also be run on a different OS than the target OS of your application, and the generated code can usually be compiled to several OSes without running the tool separately for each.

### Visual Studio configuration to use a C++ .dll in a C# project

1. Create a C# project and a C++ project in the same solution.
2. Configure the C++ project to output a .dll.
3. Add the C++ project as a dependency of the C# project in Build Dependencies -> Project Dependencies.
4. To use x64 mode, open Build -> Configuration Manager. Set the platform to x64. If the x64 platform does not exist in the list, choose "New...", choose x64 and uncheck "create new solution platform" if necessary.
5. Make sure your C# project has access to the .dll/.so file generated by your C++ project, e.g. by adding the following to the C# project's .csproj file in Visual Studio:
   `<ItemGroup>`
     `<None Include="$(SolutionDir)$(Platform)\$(ConfigurationName)\cpp_prj.dll">`
       `<CopyToOutputDirectory>Always</CopyToOutputDirectory>`
     `</None>`
   `</ItemGroup>`

## Usage

### Exporting a simple global function from C++ to C#

Simply add the `FXP` keyword before the function declaration in a C++ header file:

```c++
FXP void test(const std::string &s, int i);
```

Compile the project to run RabbitCall, switch to the C# editor, and the C++ function should be available:

```C#
class MyClass {
   	static void Main(string[] args) {
		RabbitCallApi.init();
        NGlobal.test("Hello, World!", 42);
    }
}
```

All C++ global functions will become static functions in the NGlobal C# class (name configurable). If you want to use them in C# as "globals" in the same way as in C++, use the `using static` directive:

```C#
using static NGlobal;

class MyClass {
   	static void Main(string[] args) {
		RabbitCallApi.init();
        test("Hello, World!", 42);
    }
}
```

### Classes

Add the `FXP` keyword to C++ class declarations and to member functions that you wish to export:

```C++
FXP class Test {
    int v;
public:
    FXP static Test * create() { return new Test(); }
    FXP void test(int i);
    void test2();
};
```

Then use the class from C#:

```C#
NTest obj = NTest.create();
obj.test(42);
```

Accessing member variables directly from C# is not currently supported (except by using struct types), but you can add getter/setter functions if necessary.

### Inheritance

C++:

```C++
FXP class Base {
public:
    FXP virtual void test(int i);
};

FXP class Derived : public Base {
public:
    FXP void test(int i) override;
};

FXP Derived * getDerivedObject();
```

C#:

```C#
NDerived obj = NGlobal.getDerivedObject();
obj.test(42);
NBase baseObj = obj; // Pointer can be cast to base type.
baseObj.test(42); // The overridden function gets called.
```

Also multiple inheritance is supported, but not with virtual base classes.

### Callbacks, lambda functions

C# delegates and lambda functions can be passed to C++ and then called back or stored as std::function objects:

```C++
std::function<void(int)> storedCallback;

FXP void forEachInteger(const std::function<void(int)> &callback) {
    // Invoke the callback directly.
    callback(42);
    
    // Store for later use (will keep the C# object alive until storedCallback
    // and any other copies of the given std::function are destroyed).
    storedCallback = callback;
}
```

C#:

```
NGlobal.forEachInteger(i => Console.WriteLine($"{i}"));
```

The std::function object and its copies will hold a reference to the C# delegate to prevent it from being garbage-collected while it may still be used on the C++ side (which is not guaranteed by plain P/Invoke and would cause random crashes). C++ function pointers are not supported because of this safety concern, but you can use them in custom P/Invoke methods if you are careful.

### Enums

Add an `FXP` keyword to a C++ enum to make it available in C#. Also, string/enum conversion functions will be generated that can be used in C++ code as shown below.

```C++
FXP enum class Test {
    // Default value.
	VALUE1,
    
    // Explicitly defined value.
	VALUE2 = 4,

    // This expression gets directly copied to C# code
    // => only simple expressions will work.
	VALUE3 = VALUE1 + VALUE2
};

void test(std::string v) {
    Test t = parse_Test("VALUE1"); // Convert string to enum value.
    cout << t; // "operator<<" will be auto-generated for enums.
    cout << toString_Test(t); // Convert enum value to string.
}
```

### Structs

A data struct definition in C++ can be exported as a C# struct. You can use pointers in both C++ and C# to manipulate the same structs in memory, or pass structs as function parameters or return values.

In C++ code, using the `struct` keyword instead of `class` will cause RabbitCall to export it as a pass-by-value struct instead of a pass-by-reference class (although officially in C++ these keywords do not have such meaning).

```c++
FXP struct Test1 {
    int i = 1; // Initializer only affects construction in C++.
    float f[4];
};

FXP struct Test2 {
    XMFLOAT4 v;
    Test1 test; // Nested struct.
};

FXP void printStruct(Test2 *t);
```

Manipulating the structs in C#:

```C#
Test2 t = new Test2();
t.test.i = 42;
i.v = new Vector4(1, 2, 3, 4); // C++ XMFLOAT4 corresponds to C# Vector4.
NGlobal.printStruct(&t);
```

If, for some reason, you need to manually duplicate the struct definition in C++ and C#, you can add a type mapping between them in the XML configuration file.

Alignment is also taken into consideration and field offsets in generated C# structs will be aligned accordingly, but note that C# does not naturally support alignment when e.g. allocating structs, so you will have to be careful. The tool also prohibits passing higher-than-default-aligned structs by value between C# and C++, although they can be passed by reference. It's probably best to use mainly unaligned types in the C++/C# interface e.g. in vector math.

```c++
// Define alignment explicitly with alignas(16).
FXP struct alignas(16) Test1 {
    int i;
};

// This struct gets alignment 16, because the highest-aligned member has alignment 16.
FXP struct Test2 {
    int i;
    Test1 test; // Gets aligned at offset 16.
};
```

### Strings and character encoding

C# `string` objects can be passed to C++ using either UTF-8 or UTF-16 encoding. Each C++ string type (`std::string`, `std::u16string` etc.) can be converted to/from a C# `string` in the C++/C# interface by defining the encoding and C++ character type in the XML configuration file (see the test program's configuration for details: [rabbitcall.xml](test/rabbitcall.xml)).

A convenient way to represent strings in C++ is to use UTF-8 in `std::string` objects and only convert them to another format when needed, e.g. `std::wstring` when calling Windows API (see <http://utf8everywhere.org/>). The test program's `std::string` mapping is configured this way.

If you have a Windows-only C++ codebase that uses `std::wstring` for most strings, it is also possible to map `std::wstring` to C# `string` by specifying UTF-16 in the configuration file (as shown in the test program), but this won't be portable to other operating systems where `std::wstring` uses UTF-32 instead, and UTF-8 has other benefits as well.

### Exceptions

All exceptions derived from std::exception that are thrown by a C++ function are propagated to the calling C# function as a C# exception. The C# exception stack trace will be limited to the C# stack, but the message will be copied from the C++ exception (from the std::exception::what() function).

```C++
FXP void test() {
    throw std::logic_error("test_exception");
}
```

C#:

```C#
try {
    NGlobal.test();
}
catch (Exception e) {
	Console.WriteLine(e.Message); // Outputs "test_exception".
}
```

The exception check may add a small overhead to the C# => C++ function call, which is only about 1 CPU cycle on Windows x64. The check can be omitted by the C++ `noexcept` keyword:

```C++
FXP void test() noexcept;
```

Exceptions from C# callback methods are propagated through C++ to the original calling C# method.

### Source code comments

Comments above C++ classes, functions, struct members and other declarations are copied to the generated C# and HLSL/GLSL declarations.

```C++
// This is a class.
FXP class Test {
public:
    /*
     * This is a function.
     */
    FXP void test(int i);
};
```

### Namespaces

Declarations inside a C++ namespace block will appear in C# inside the same namespace:

```C++
namespace TestNamespace {
    FXP class Test {
    public:
        void doSomething();
    };

    FXP Test * createTest();
};
```

C#:

```C#
// Global functions will be put in an NGlobal class inside the namespace.
TestNamespace.NTest t = TestNamespace.NGlobal.test();
t.doSomething();
```

RabbitCall supports only namespace blocks that are defined inside the same header file, i.e. using an `#include` directive inside a namespace block can cause problems.

C++ namespace aliases and `using namespace` directives are not processed by the tool. Instead, it is assumed that the included header files may have any `using` directives for any namespaces, and any partially-qualified name is accepted that could refer to a known type. If a type name encountered e.g. in an exported function declaration is ambiguous to the tool (even if not to the C++ compiler), the tool shows an error, in which case you should use a fully-qualified name instead.

You can also enclose all exported items in the C# project inside a single namespace by changing the XML configuration file.

### HLSL/GLSL

C++ structs can be exported to HLSL/GLSL structs and constants buffers / uniform blocks by adding parameter `hlsl` or `glsl` to the export specification. When the `FXP` keyword requires parameters, use the `FXPP` keyword instead. The same structs and enums will be available in C# as well.

Because HLSL/GLSL have different packing rules than C++, you must sometimes add padding yourself to avoid creating a C++ struct that would be impossible to present in HLSL/GLSL. Most types are aligned at 4-byte boundaries and in HLSL, a single element is aligned at the next 16-byte boundary if it would otherwise cross this boundary. In GLSL, the `std140` packing rules are used, which are similar to HLSL rules, except e.g. vec3 is always aligned by 16 even if it would fit at the end of the previous 16-byte chunk.

```C++
FXPP(hlsl, glsl) struct TestStruct {
    XMFLOAT4 v1;
    XMFLOAT2 v2;
    int i;
    float f;
};
```

A C++ struct can be converted to a HLSL constant buffer by using the `cbuffer` parameter, optionally adding a prefix to each field and/or specifying a register.

```C++
FXPP(hlsl(cbuffer, prefix(g_), register(b0))) struct alignas(16) TestConstantBuffer {
	XMMATRIX viewMatrix;
	XMFLOAT2 v1 = float2(1, 2);
	float v2 = 0;
};
```

The resulting HLSL:

```hlsl
struct TestStruct {
	float4 v1;
	float2 v2;
    int i;
    float f;
};

cbuffer TestConstantBuffer : register(b0) {
	matrix g_viewMatrix;
	float2 g_v1;
	float g_v2;
};
```

To create a GLSL uniform block, use the `storage(uniform)` parameter. You can also specify a `prefix` for all field names, an `instanceName` through which the fields are accessed, and a `binding` index.

```C++
FXPP(glsl(storage(uniform), prefix(g_), binding(0))) struct alignas(16) TestBlock {
    XMMATRIX viewMatrix;
	XMFLOAT2 v1 = float2(1, 2);
	float v2 = 0;
};
```

```glsl
layout(std140, binding = 0) uniform TestBlock {
	mat4 g_viewMatrix;
	vec2 g_v1;
	float g_v2;
};
```

Also C++ enums can be exported to HLSL/GLSL:

```hlsl
FXPP(hlsl(prefix(color_)), glsl(prefix(color_))) enum class Color {
	RED = 1,
	GREEN = 2,
	BLUE = 3,
};
```

Each enum entry becomes a constant in HLSL/GLSL, with the given prefix. In GLSL, the `static` keyword is omitted.

```hlsl
static const int color_RED = 1;
static const int color_GREEN = 2;
static const int color_BLUE = 3;
```

### Adding custom C# methods and fields to exported classes

You can use the C# partial classes/structs feature or C# extension methods to add stuff to the auto-generated classes and structs without modifying the generated file directly (because it would be overwritten). Note that both pass-by-reference classes and pass-by-value structs are represented as "structs" in C#.

```
namespace CsNamespace {
	public unsafe partial struct NTest {
		public void callTest() {
			test();
		}
	};
}
```

### Example project

See the test project's source code for more complete working examples.

## Performance

Performance is in most cases similar to raw P/Invoke calls and generally only matters if you are doing millions of function calls per second.

On Windows 10 using .Net Core 3.1 or .Net Framework 4.8, simple function calls from C# to C++ without parameters can be expected to have 5-10 CPU cycles overhead, and 25 cycles in the other direction. Passing simple parameters and structs between C# and C++ has about the same overhead as between two C++ functions, i.e. it can be even zero if passed in registers, or a few cycles if passed on the stack. The `vectorcall` calling convention is not supported between C# and C++. Passing strings or delegates typically has 100-500 CPU cycles overhead.

On Ubuntu Linux 18.04 using .Net Core 3.1, the performance is somewhat worse: 80 CPU cycles for C# to C++ calls, 260 cycles for C++ to C# calls, 500 cycles for passing short strings and 2000 for passing delegates.

On Mono, the P/Invoke calls used by RabbitCall are significantly slower than on .Net Core/Framework, and simple function calls may take hundreds of CPU cycles. This might not be a limiting factor in most cases, but you might want to use Mono's "internal calls" in the most performance-critical functions.

## Large project considerations

Because the tool can process about 100 MB of C++ source per second, and does not have to process 3rd party headers, running time of the tool is unlikely to be a concern. If it nevertheless is, you can consider splitting C++ libraries into multiple parts, or running the tool manually only when needed.

What can be a concern, however, is that typical C++ compilers are much slower, and if all auto-generated code in a huge project is put in a single pair of .h and .cpp files, recompiling it can be slow. The solution is to create "partitions" in the XML configuration file to split the code in several parts, so that separate .h/.cpp files are generated for each partition. See comments in the test program's configuration file for details.

If there are multiple partitions, the first partition is a special "main" partition that contains some common definitions used by all partitions. Any change to the exported C++ interface in the main partition will cause all code dependent on any partition to be recompiled. Therefore, the first partition in a multi-partition setup should be left empty or have rarely-changing definitions only.

## Additional tips

- If you have multiple C++ .dlls, you can use them all in a single C# program by creating a separate XML configuration file for each and configuring different generated-file-paths and a different C# namespace. You have to invoke `RabbitCallApi.init()` for each of them separately.
- If the `includeSourceHeadersInGeneratedCpp` parameter is not enough to get all necessary header files included in the generated .cpp file, try adding the #include directives either to `generatedCppFilePrologue` in the XML configuration or to some of your header files explicitly.
- The tool is normally triggered by a pre-build event whenever the C++ project is built because a source file has been modified. If you modify the configuration XML, you should also touch a source file to trigger a build, or set `DisableFastUpToDateCheck` to `true` in a `PropertyGroup` in the Visual Studio project file (which can slow down rebuild checks for large projects though).

## Building RabbitCall yourself

RabbitCall is written in portable C++17 using CMake, and can be built for other platforms besides those for which binaries are provided.

Libraries used:

- [Boost 1.72](https://www.boost.org/) (must be installed separately for building).
- [TinyXML2](https://github.com/leethomason/tinyxml2) (embedded in code, slightly modified).
- [UTF8-CPP](http://utfcpp.sourceforge.net/) (embedded in code).
- [GLFW 3.3.2](https://www.glfw.org/) (needed in the test program only, install and configure paths in project files if you want to run OpenGL tests).

### Windows

In Visual Studio 2019, open CMake project by choosing File -> Open -> Folder -> select the main folder with the CMakeLists.txt file. Then edit the generated CMakeSettings.json file and add the following section to the top level (and set the path to your Boost installation):

```json
"environments": [
  {
    "RABBITCALL_SOURCE": "src",
    "BOOST_ROOT": "C:/boost_1_72_0"
  }
],
```

### Other platforms

You can use GCC/G++ 9 (also 8 might be sufficient) and the following bash script (edit the paths if necessary):

```sh
export SOURCE_DIR=src
export CMAKE_C_COMPILER=/usr/bin/gcc-9
export CMAKE_CXX_COMPILER=/usr/bin/g++-9
export BOOST_ROOT=/usr/local/bin/boost_1_72_0
cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo .
make
```

