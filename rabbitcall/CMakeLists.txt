cmake_minimum_required (VERSION 3.8)
project ("rabbitcall")

if(NOT CMAKE_VERSION VERSION_LESS "3.12")
	cmake_policy(SET CMP0074 OLD)
endif()

if(MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")

	set(compilerFlagVars
        CMAKE_CXX_FLAGS
        CMAKE_CXX_FLAGS_DEBUG
        CMAKE_CXX_FLAGS_RELEASE
        CMAKE_CXX_FLAGS_RELWITHDEBINFO
        CMAKE_C_FLAGS
        CMAKE_C_FLAGS_DEBUG
        CMAKE_C_FLAGS_RELEASE
		CMAKE_C_FLAGS_RELWITHDEBINFO
	)
	foreach(compilerFlagVar ${compilerFlagVars})
		string(REPLACE "/MD" "/MT" ${compilerFlagVar} "${${compilerFlagVar}}")
	endforeach()
endif()

set(CMAKE_C_COMPILER $ENV{CMAKE_C_COMPILER})
set(CMAKE_CXX_COMPILER $ENV{CMAKE_CXX_COMPILER})
set(SOURCE_DIR $ENV{SOURCE_DIR})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(Boost_USE_STATIC_LIBS ON) 
set(Boost_USE_MULTITHREADED ON)  
set(Boost_USE_STATIC_RUNTIME ON)
find_package(Boost 1.72 COMPONENTS REQUIRED)
include_directories(${Boost_INCLUDE_DIR} ${SOURCE_DIR})

file(GLOB_RECURSE SOURCE_FILES ${SOURCE_DIR}/*.cpp ${SOURCE_DIR}/*.h)

add_executable(rabbitcall ${SOURCE_FILES})
target_link_libraries(rabbitcall LINK_PUBLIC ${Boost_LIBRARIES})

if(MSVC)
else()
	set(THREADS_PREFER_PTHREAD_FLAG ON)
	find_package(Threads REQUIRED)
	target_link_libraries(rabbitcall Threads::Threads)
endif()
