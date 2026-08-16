cmake_minimum_required(VERSION 3.12)

if (NOT DEFINED VERSION)
    if (DEFINED ENV{VERSION})
        set(VERSION $ENV{VERSION})
    else ()
        set(VERSION "0.0.1")
    endif ()
endif ()

if (NOT DEFINED RUN_INFO)
    if (DEFINED ENV{RUN_INFO})
        set(RUN_INFO $ENV{RUN_INFO})
    else ()
        set(RUN_INFO "local")
    endif ()
endif ()

project(UTBotCpp DESCRIPTION "Tool that generates unit test by C/C++ source code, trying to reach all branches and maximize code coverage"
        HOMEPAGE_URL "https://www.utbot.org" VERSION ${VERSION})
set(PROJECT_ORG "UnitTestBot")

message("Project: ${PROJECT_NAME}")
message("Organisation: ${PROJECT_ORG}")
message("Homepage: ${CMAKE_PROJECT_HOMEPAGE_URL}")
message("Version: ${CMAKE_PROJECT_VERSION}")
message("Run from: ${RUN_INFO}")
include_directories(${PROJECT_BINARY_BIN})

configure_file(config.h.in config.h @ONLY)

if (UNIX)
    add_compile_definitions(_LINUX)
endif ()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# clang-cl takes the MSVC spelling of everything, and link.exe understands none
# of the GNU flags below, so each of these is asked for in the dialect of the
# driver that will read it.
if (MSVC)
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Od /Zi")
else ()
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -g -fstandalone-debug -fno-discard-value-names -fno-elide-constructors")
    # -export-dynamic keeps the symbol table for backtraces; there is no
    # equivalent for a PE image, which exports only what it declares.
    set(CMAKE_EXE_LINKER_FLAGS "-Wl,-export-dynamic")
endif ()

include(cmake/build_type.cmake)

enable_testing()
if (MSVC)
    add_compile_options(/EHsc /GR)
else ()
    add_compile_options(-frtti -fexceptions)
endif ()

# Appended, never assigned. A toolchain file gets to set this first -- vcpkg's
# does, and that is how anything it installed is found at all -- so replacing it
# discards those paths, and the packages below then cannot be found however
# correctly they were built.
if (NOT "$ENV{UTBOT_INSTALL_DIR}" STREQUAL "")
    list(APPEND CMAKE_PREFIX_PATH $ENV{UTBOT_INSTALL_DIR})
endif ()
find_package(Clang REQUIRED)

# Official Windows LLVM archives record the DIA SDK path from the machine that
# produced the archive (currently VS 2022 Enterprise).  Replace that stale path
# with the DIA SDK installed alongside the compiler used for this build.
if (WIN32 AND TARGET LLVMDebugInfoPDB AND NOT "$ENV{VSINSTALLDIR}" STREQUAL "")
    set(UTBOT_DIAGUIDS "$ENV{VSINSTALLDIR}/DIA SDK/lib/amd64/diaguids.lib")
    if (EXISTS "${UTBOT_DIAGUIDS}")
        get_target_property(UTBOT_PDB_LIBRARIES LLVMDebugInfoPDB INTERFACE_LINK_LIBRARIES)
        foreach (UTBOT_PDB_LIBRARY IN LISTS UTBOT_PDB_LIBRARIES)
            if (UTBOT_PDB_LIBRARY MATCHES "DIA SDK.*/diaguids\\.lib$")
                list(REMOVE_ITEM UTBOT_PDB_LIBRARIES "${UTBOT_PDB_LIBRARY}")
            endif ()
        endforeach ()
        list(PREPEND UTBOT_PDB_LIBRARIES "${UTBOT_DIAGUIDS}")
        set_property(TARGET LLVMDebugInfoPDB PROPERTY
                INTERFACE_LINK_LIBRARIES "${UTBOT_PDB_LIBRARIES}")
    endif ()
endif ()
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${LLVM_CMAKE_DIR}")
include(AddLLVM)
include_directories(${LLVM_INCLUDE_DIRS})
include_directories(${CLANG_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})
add_definitions(${CLANG_DEFINITIONS})

# LLVM is built without exceptions or RTTI and AddLLVM propagates -fno-exceptions
# to anything that includes it, landing after the -fexceptions requested above.
# The server needs both -- it catches by reference throughout and the gRPC and
# CLI11 headers do too -- so they are re-asserted here, after LLVM has had its
# say.
if (MSVC)
    add_compile_options(/EHsc /GR)
else ()
    add_compile_options(-fexceptions -frtti)
endif ()

# Where a development build looks for the tools it spawns. isDevEnvironment()
# tests whether this path exists, so a distribution has to be built with it
# empty: otherwise the binary prefers a path from the machine that built it,
# and only falls back to its own location on machines where that path happens
# not to exist -- which is every machine except the build one, so the mistake
# does not show up until it does.
set(UTBOT_DEV_ROOT_DIR "$ENV{UTBOT_ALL}" CACHE STRING
    "Root the server resolves its tools against; empty for a relocatable build")
add_definitions(-DUTBOT_DEV_ROOT_DIR="${UTBOT_DEV_ROOT_DIR}")

set(GRPC_PATH $ENV{GRPC_PATH})
# Only when there is one to add: unset, this contributed "/lib/cmake/grpc" and
# took the rest of the prefix path with it.
if (NOT "${GRPC_PATH}" STREQUAL "")
    list(APPEND CMAKE_PREFIX_PATH ${GRPC_PATH}/lib/cmake/grpc ${GRPC_PATH}/lib/cmake/protobuf)
endif ()
if (NOT "${GRPC_PATH}" STREQUAL "")
    include_directories(${GRPC_PATH}/include)
endif ()

find_package(Threads REQUIRED)
find_package(Protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)

add_definitions(-DLOGURU_WITH_STREAMS=1)
# Threads::Threads is already required above and carries whatever the platform
# needs; -ldl has no Windows counterpart, since a PE image loads libraries
# through the kernel rather than through a separate library.
if (NOT MSVC)
    SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lpthread -ldl")
endif ()

get_filename_component(UTBOT_DIR ${PROJECT_SOURCE_DIR} DIRECTORY)

set(SUBMODULES_DIR "${UTBOT_DIR}/submodules")
set(LOGGER_DIR "${SUBMODULES_DIR}/loguru")
set(LOGGER_AUX_DIR "${PROJECT_SOURCE_DIR}/logger-aux")
add_library(loguru ${LOGGER_DIR}/loguru.cpp ${LOGGER_AUX_DIR}/loguru_debug.cpp)
target_include_directories(loguru PUBLIC ${LOGGER_DIR})
include_directories(${LOGGER_AUX_DIR})

set(JSON_PATH "${SUBMODULES_DIR}/json")
include_directories(${JSON_PATH}/src)

set(TSL_PATH "${SUBMODULES_DIR}/ordered-map")
message("tsl include from here: ${TSL_PATH}/include")
include_directories(${TSL_PATH}/include)

set(PARALLEL_HASHMAP_PATH "${SUBMODULES_DIR}/parallel-hashmap")
include_directories(${PARALLEL_HASHMAP_PATH})

# The parser is built from the KTest reader in the pinned source submodule. Do not
# derive this from UTBOT_ALL: that is the runtime/install layout, and on Windows
# its backslashes are also parsed as CMake escapes when used as a source path.
set(KLEE_PATH "${UTBOT_DIR}/submodules/klee")
include_directories(${KLEE_PATH}/include)

# KLEE itself is not linked -- it is spawned as a process -- but its test cases
# still have to be read back, and the .ktest reader is one self-contained C++
# file with no dependency on the rest of KLEE. Compiling just that keeps the
# format definition and the code parsing it in step without needing a KLEE build.
add_library(kleeKTest STATIC ${KLEE_PATH}/lib/Basic/KTest.cpp)
target_include_directories(kleeKTest PUBLIC ${KLEE_PATH}/include)

set(CLI_PATH $ENV{CLI_PATH})
include_directories(${CLI_PATH})

file(GLOB FETCHERS_SRC
        "${PROJECT_SOURCE_DIR}/src/fetchers/*"
        )

file(GLOB ALL_SOURCES
        "${PROJECT_SOURCE_DIR}/src/*"
        "${PROJECT_SOURCE_DIR}/src/building/*"
        "${PROJECT_SOURCE_DIR}/src/clang-utils/*"
        "${PROJECT_SOURCE_DIR}/src/tasks/*"
        "${PROJECT_SOURCE_DIR}/src/coverage/*"
        "${PROJECT_SOURCE_DIR}/src/environment/*"
        "${PROJECT_SOURCE_DIR}/src/exceptions/*"
        "${PROJECT_SOURCE_DIR}/src/fetchers/*"
        "${PROJECT_SOURCE_DIR}/src/printers/*"
        "${PROJECT_SOURCE_DIR}/src/streams/*"
        "${PROJECT_SOURCE_DIR}/src/streams/coverage/*"
        "${PROJECT_SOURCE_DIR}/src/streams/stubs/*"
        "${PROJECT_SOURCE_DIR}/src/streams/tests/*"
        "${PROJECT_SOURCE_DIR}/src/testgens/*"
        "${PROJECT_SOURCE_DIR}/src/utils/*"
        "${PROJECT_SOURCE_DIR}/src/utils/path/*"
        "${PROJECT_SOURCE_DIR}/src/utils/stats/*"
        "${PROJECT_SOURCE_DIR}/src/stubs/*"
        "${PROJECT_SOURCE_DIR}/src/visitors/*"
        "${PROJECT_SOURCE_DIR}/src/commands/*"
        "${PROJECT_SOURCE_DIR}/src/types/*"
        "${PROJECT_SOURCE_DIR}/resources/*"
        ${FETCHERS_SRC}
        )

set(PROTO_DIR "${PROJECT_SOURCE_DIR}/proto")
set(PROTO_COMPILE_DIR "${CMAKE_CURRENT_BINARY_DIR}/protobuf")
file(GLOB PROTO_FILES "${PROTO_DIR}/*.proto")
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/protobuf)

set(ALL_PROTO_GENERATED_SOURCES)
foreach (proto_file ${PROTO_FILES})
    get_filename_component(proto_src ${proto_file} PATH)
    get_filename_component(proto_filename ${proto_file} NAME_WE)
    set(proto_outputs
            ${PROTO_COMPILE_DIR}/${proto_filename}.grpc.pb.cc
            ${PROTO_COMPILE_DIR}/${proto_filename}.grpc.pb.h
            ${PROTO_COMPILE_DIR}/${proto_filename}.pb.cc
            ${PROTO_COMPILE_DIR}/${proto_filename}.pb.h)
    add_custom_command(
            OUTPUT ${proto_outputs}
            COMMAND $<TARGET_FILE:protobuf::protoc>
            ARGS --grpc_out ${PROTO_COMPILE_DIR}
                 --cpp_out ${PROTO_COMPILE_DIR}
                 -I ${proto_src}
                 --plugin=protoc-gen-grpc=$<TARGET_FILE:gRPC::grpc_cpp_plugin>
                 ${proto_file}
            DEPENDS ${proto_file} protobuf::protoc gRPC::grpc_cpp_plugin
            COMMENT "Generating C++ and gRPC sources from ${proto_filename}.proto"
            VERBATIM)
    list(APPEND ALL_PROTO_GENERATED_SOURCES ${proto_outputs})
    set_property(SOURCE main.cpp APPEND PROPERTY OBJECT_DEPENDS
            ${PROTO_COMPILE_DIR}/${proto_filename}.grpc.pb.h)
endforeach ()

include_directories("${CMAKE_CURRENT_BINARY_DIR}")

# KLEE is spawned as a process rather than linked in, so the run_klee package
# that UnitTestBot's fork exports is no longer needed. That is what lets the
# server build against a KLEE with no such library, and on a platform with no
# fork() to call it from.

option(ENABLE_PRECOMPILED_HEADERS "Enable precompiled headers" ON)

if (ENABLE_PRECOMPILED_HEADERS)
    set_source_files_properties(${ALL_PROTO_GENERATED_SOURCES} PROPERTIES SKIP_PRECOMPILE_HEADERS ON)
endif ()

################################################################################
# Library
################################################################################
add_library(UTBotCppLib STATIC ${ALL_PROTO_GENERATED_SOURCES} ${ALL_SOURCES})
target_include_directories(UTBotCppLib PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        resources
        thirdparty/ordered-map)

target_link_libraries(UTBotCppLib PUBLIC clangTooling clangBasic clangASTMatchers clangRewriteFrontend
        gRPC::grpc++_reflection
        gRPC::grpc++
        protobuf::libprotobuf
        loguru
        kleeKTest
        )
if (ENABLE_PRECOMPILED_HEADERS)
    target_precompile_headers(UTBotCppLib PUBLIC pch.h)
endif ()

################################################################################
# Executable
################################################################################
# add_llvm_executable runs llvm_update_compile_flags, which applies LLVM's own
# -fno-exceptions and -fno-rtti to the target. The server needs both, and so do
# the gRPC and CLI11 headers it includes; these are the flags LLVM documents for
# a target to say so.
set(LLVM_REQUIRES_EH ON)
set(LLVM_REQUIRES_RTTI ON)
add_llvm_executable(utbot main.cpp)
target_link_libraries(utbot PUBLIC
        UTBotCppLib
        loguru
        )

################################################################################
# Testing
################################################################################
option(ENABLE_UNIT_TESTS "Enable unit tests" ON)

if (ENABLE_UNIT_TESTS)
    message(STATUS "Unit tests enabled")

    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    add_subdirectory($ENV{UTBOT_ALL}/gtest
            ${CMAKE_CURRENT_BINARY_DIR}/googletest-build
            EXCLUDE_FROM_ALL)
    file(GLOB ALL_TESTS "${PROJECT_SOURCE_DIR}/test/framework/*.cpp")

    add_executable(
            UTBot_UnitTests
            ${ALL_TESTS}
    )

    target_include_directories(UTBot_UnitTests PUBLIC src src/include $ENV{UTBOT_ALL}/gtest/googletest)
    target_link_libraries(
            UTBot_UnitTests
            PUBLIC
            gtest
            UTBotCppLib
    )

    if (ENABLE_PRECOMPILED_HEADERS)
        target_precompile_headers(UTBot_UnitTests PRIVATE [["gtest/gtest.h"]])
    endif ()

    add_test(NAME test COMMAND UTBot_UnitTests)
else ()
    message(STATUS "Unit tests disabled")
endif ()

################################################################################
# Miscellaneous install
################################################################################
install(TARGETS utbot
        DESTINATION ${CMAKE_INSTALL_PREFIX}
        )
