cmake_minimum_required(VERSION 3.2)

set(CMAKE_CXX_STANDARD 20)

find_package(GTest REQUIRED)

file(GLOB TEST_FILES "${CMAKE_CURRENT_SOURCE_DIR}/test*.cpp")

message(${CMAKE_CURRENT_SOURCE_DIR})

add_executable(crypto-tests ${TEST_FILES})
enable_testing()
add_test(crypto-tests "./test")
target_link_libraries(crypto-tests contract-crypto Threads::Threads ${GTEST_LIBRARIES} gtest_main contract-utils sdk_external)
target_include_directories(contract-crypto PUBLIC ../include)