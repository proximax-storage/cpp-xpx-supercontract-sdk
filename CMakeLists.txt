cmake_minimum_required(VERSION 3.9)
project(cpp-xpx-supercontract-sdk VERSION 1.0.0)
include(CMakeGlobalSettings.cmake)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if(PROJECT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(SIRIUS_CONTRACT_MASTER_PROJECT ON)
endif()

set(SKIP_PERFORMANCE_COMPARISON ON)
set(SKIP_PORTABILITY_TEST ON)
set(BUILD_DOC OFF)
set(BUILD_SANDBOX OFF)

### setup boost
find_package(Boost COMPONENTS atomic system date_time regex timer chrono log thread filesystem program_options random stacktrace_backtrace REQUIRED)

supercontract_sdk_third_party_lib(cmake/grpc.cmake)
supercontract_sdk_third_party_lib(cmake/expected.cmake)
supercontract_sdk_third_party_lib(cmake/cereal.cmake)
supercontract_sdk_third_party_lib(cmake/magic_enum.cmake)

option(SIRIUS_CONTRACT_BUILD_TESTS "Build sirius contract tests" ${SIRIUS_CONTRACT_MASTER_PROJECT})
if(SIRIUS_CONTRACT_BUILD_TESTS)
    enable_testing()
    configure_file(cmake/CTestCustom.cmake ${CMAKE_BINARY_DIR})
endif()

add_subdirectory(libs)
add_subdirectory(src)

if(SIRIUS_CONTRACT_BUILD_TESTS)
    add_subdirectory(test)
endif()