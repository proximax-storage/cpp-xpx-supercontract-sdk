cmake_minimum_required(VERSION 3.9)
project(cpp-xpx-supercontract-sdk VERSION 1.0.0)
include(CMakeGlobalSettings.cmake)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

### setup boost
find_package(Boost COMPONENTS atomic system date_time regex timer chrono log thread filesystem program_options random stacktrace_backtrace REQUIRED)

include(FetchContent)

supercontract_sdk_third_party_lib(cmake/grpc.cmake)
supercontract_sdk_third_party_lib(cmake/expected.cmake)
supercontract_sdk_third_party_lib(cmake/cereal.cmake)
supercontract_sdk_third_party_lib(cmake/magic_enum.cmake)

add_subdirectory(libs)
#add_subdirectory(src)
#add_subdirectory(test)