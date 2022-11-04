cmake_minimum_required(VERSION 3.2)

set(CMAKE_CXX_STANDARD 20)

find_package(GTest REQUIRED)

file(GLOB TEST_FILES "${CMAKE_CURRENT_SOURCE_DIR}/test*.cpp")

add_executable(virtualMachine-tests ${TEST_FILES} TestUtils.cpp MockInternetHandler.cpp MockStorageHandler.cpp FaultyMockStorageHandler.cpp)
enable_testing()
add_test(virtualMachine-tests "./test")
target_link_libraries(virtualMachine-tests contract-virtualMachine Threads::Threads ${GTEST_LIBRARIES} gtest_main)

