#pragma once

// Add the FXP keyword before class/struct/enum/function declarations to export them to C#.
// Use the FXPP keyword when you need parameters.
#define FXP
#define FXPP(...)

#ifdef _MSC_VER
#define RC_EXPORT __declspec(dllexport)
#else
#define RC_EXPORT __attribute__((visibility("default")))
#endif
