cmake_minimum_required(VERSION 3.2)

project(internet-tests)

set(CMAKE_CXX_STANDARD 20)

find_package(GTest REQUIRED)

file(GLOB TEST_FILES "${CMAKE_CURRENT_SOURCE_DIR}/test*.cpp")

add_executable(internet-tests ${TEST_FILES})
enable_testing()
add_test(supercontract-tests "./test")
target_link_libraries(internet-tests contract-internet Threads::Threads ${GTEST_LIBRARIES} gtest_main)

