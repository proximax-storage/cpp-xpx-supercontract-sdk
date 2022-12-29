cmake_minimum_required(VERSION 3.9)
project(cpp-xpx-supercontract-sdk VERSION 1.0.0)
include(CMakeGlobalSettings.cmake)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)
set(ABSL_PROPAGATE_CXX_STD ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(BUILD_SHARED_LIBS ON)

### setup boost
find_package(Boost COMPONENTS atomic system date_time regex timer chrono log thread filesystem program_options random stacktrace_backtrace REQUIRED)

include(FetchContent)
set(gRPC_SSL_PROVIDER "package" CACHE STRING "Provider of ssl library")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_TMP ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/../lib)
option(BUILD_SHARED_LIBS "build shared libraries" ON)
FetchContent_Declare(
        grpc
        GIT_REPOSITORY https://github.com/grpc/grpc.git
        GIT_TAG        v1.49.1)
FetchContent_MakeAvailable(grpc)
option(BUILD_SHARED_LIBS "build shared libraries" OFF)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY_TMP})

FetchContent_Declare(
        expected
        GIT_REPOSITORY https://github.com/TartanLlama/expected
        GIT_TAG        v1.0.0)
FetchContent_MakeAvailable(expected)

FetchContent_Declare(
        magic_enum
        GIT_REPOSITORY https://github.com/Neargye/magic_enum
        GIT_TAG        v0.8.1)
FetchContent_MakeAvailable(magic_enum)

FetchContent_Declare(
        cereal
        GIT_REPOSITORY https://github.com/USCiLab/cereal.git
        GIT_TAG        v1.3.2)
FetchContent_MakeAvailable(cereal)

# Since FetchContent uses add_subdirectory under the hood, we can use
# the grpc targets directly from this build.
set(_PROTOBUF_LIBPROTOBUF libprotobuf)
set(_REFLECTION grpc++_reflection)
set(_PROTOBUF_PROTOC $<TARGET_FILE:protoc>)
set(_GRPC_GRPCPP grpc++)
if(CMAKE_CROSSCOMPILING)
    find_program(_GRPC_CPP_PLUGIN_EXECUTABLE grpc_cpp_plugin)
else()
    set(_GRPC_CPP_PLUGIN_EXECUTABLE $<TARGET_FILE:grpc_cpp_plugin>)
endif()

add_subdirectory(libs)
#add_subdirectory(src)
#add_subdirectory(test)