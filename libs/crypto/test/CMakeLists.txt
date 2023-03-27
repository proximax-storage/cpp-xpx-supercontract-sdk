cmake_minimum_required(VERSION 3.9)

find_package(GTest REQUIRED)

file(GLOB TEST_FILES "${CMAKE_CURRENT_SOURCE_DIR}/test*.cpp")

message(${CMAKE_CURRENT_SOURCE_DIR})

add_executable(sirius.contract.crypto.tests ${TEST_FILES})
supercontract_sdk_add_test(sirius.contract.crypto.tests)
target_link_libraries(sirius.contract.crypto.tests sirius.contract.crypto Threads::Threads ${GTEST_LIBRARIES} gtest_main sirius.contract.utils)
target_include_directories(sirius.contract.crypto.tests PUBLIC ../include)
target_include_directories(sirius.contract.crypto.tests PUBLIC ../src)