cmake_minimum_required(VERSION 3.2)

set(CMAKE_CXX_STANDARD 20)

find_package(GTest REQUIRED)

file(GLOB TEST_FILES "${CMAKE_CURRENT_SOURCE_DIR}/test*.cpp")

add_executable(messenger-tests ${TEST_FILES} TestUtils.cpp)
enable_testing()
add_test(messenger-tests "./test")
target_link_libraries(messenger-tests contract-messenger Threads::Threads ${GTEST_LIBRARIES} gtest_main)

