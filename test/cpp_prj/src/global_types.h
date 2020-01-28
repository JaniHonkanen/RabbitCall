#pragma once

// This file contains definitions of structs that are mapped to C# types in the configuration file.
// A more preferable way is to auto-export the structs using the FXP keyword when possible,
// but sometimes it may be necessary to map structs e.g. to built-in C# types that cannot be auto-exported.
// These structs must be made available to the main partition's generated files by e.g. including them
// in precompiled headers or other headers used in the main partition.

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Custom data types used in GPU programming that are mapped to corresponding types in C# and HLSL by the configuration file.
// Other similar types (e.g. XMFLOAT4/3/2) can be mapped to the same C#/HLSL types at the same time.

struct float4 {
	float x, y, z, w;

	float4() = default;
	float4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}

	__m128 toM128() const {
		return _mm_loadu_ps(&x);
	}
};

struct float3 {
	float x, y, z;

	float3() = default;
	float3(float x, float y, float z) : x(x), y(y), z(z) {}

	__m128 toM128() const {
		return _mm_movelh_ps(_mm_unpacklo_ps(_mm_load_ss(&x), _mm_load_ss(&y)), _mm_load_ss(&z));
	}
};

struct float2 {
	float x, y;

	float2() = default;
	float2(float x, float y) : x(x), y(y) {}

	__m128 toM128() const {
		return _mm_unpacklo_ps(_mm_load_ss(&x), _mm_load_ss(&y));
	}
};

struct float4x4 {
	float m[16];
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// A structure whose definition is replicated manually in C++ and C# and is mapped in the configuration file.

struct CustomSharedStruct {
	int i1;
	float4 v1;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
