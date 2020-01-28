#pragma once

// Disable std::byte to avoid errors from redefinition in windows.h when it is included (in the platform-specific file) after the "using namespace std" directive.
#define _HAS_STD_BYTE 0  // NOLINT(cppcoreguidelines-macro-usage)

#pragma warning(disable : 4996)
#pragma warning(disable : 26444)

#pragma warning(push)
#pragma warning(disable : 26454)
#pragma warning(disable : 26495)
#pragma warning(disable : 26812)

#include <cstdio>
#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <cstddef>
#include <set>
#include <unordered_set>
#include <map>
#include <unordered_map>
#include <mutex>
#include <condition_variable>
#include <deque>
#include <limits>
#include <functional>
#include <filesystem>
#include <thread>

#include <boost/algorithm/string.hpp>

using namespace std;

#include "util/xml/tinyxml2.h"
#include "util/char_encoding/utf8.h"

#pragma warning(pop)

static const int applicationErrorReturnCode = 1;

#include "util/util.h"
#include "platform_specific.h"
#include "util/log.h"
#include "util/byte_buffer.h"
#include "util/string_builder.h"
#include "util/count_map.h"
#include "util/stop_watch.h"
#include "util/line_numbers.h"
#include "util/error_list.h"
#include "util/thread_pool.h"
#include "util/file_util.h"
#include "config.h"
#include "cpp/type_map.h"
#include "cpp/cpp_file.h"
#include "cpp/cpp_source_directory.h"
#include "cpp/cpp_parse_util.h"
#include "cpp/cpp_tokenizer.h"
#include "cpp/cpp_items.h"
#include "cpp/cpp_function_and_variable_parser.h"
#include "cpp/cpp_file_parser.h"
#include "cpp/cpp_partition.h"
#include "cpp/cpp_project.h"
#include "output/output_file_generator.h"
#include "output/header_output_generator.h"
#include "output/cpp_output_generator.h"
#include "output/cs_output_generator.h"
#include "output/hlsl_output_generator.h"
#include "output/glsl_output_generator.h"
#include "application.h"


