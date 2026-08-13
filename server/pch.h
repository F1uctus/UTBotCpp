#pragma once

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#endif

// stl
#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <exception>
#include <filesystem>
#include <forward_list>
#include <fstream>
#include <functional>
#include <future>
#include <iterator>
#include <list>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <regex>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <tuple>
#include <type_traits>
#ifndef _WIN32
#include <unistd.h>
#endif
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

// CLI11 includes Windows shell headers. Parse those before project headers
// introduce protobuf enum names such as BOOL and CHAR into lookup; otherwise
// the generated names are ambiguous with the Windows SDK typedefs.
#include <CLI11.hpp>
#include <rang.hpp>

// grpc
#include <grpcpp/grpcpp.h>
#include <grpcpp/server_context.h>
#include <protobuf/testgen.grpc.pb.h>
#include <protobuf/testgen.pb.h>
#include <protobuf/util.pb.h>

// loguru
#include "loguru.h"

// json
#include "json.hpp"

// tsl
#include <tsl/ordered_map.h>
#include <tsl/ordered_set.h>
