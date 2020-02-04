
#ifndef PCH_H
#define PCH_H

#define _HAS_STD_BYTE 0

#include <iostream>
#include <cstring>
#include <sstream>
#include <fstream>
#include <functional>
#include <memory>
#include <xmmintrin.h>
#include <emmintrin.h>

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef _MSC_VER

#include <intrin.h>
#define VECTORCALL __vectorcall

#else

#include <x86intrin.h>
#define VECTORCALL

#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "rabbitcall/rabbitcall.h"
#include "util.h"
#include "global_types.h"

#endif //PCH_H
