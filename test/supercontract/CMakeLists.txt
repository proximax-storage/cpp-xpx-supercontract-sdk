cmake_minimum_required(VERSION 3.9)

project(supercontract-tests)

set(CMAKE_CXX_STANDARD 20)

find_package(GTest REQUIRED)

file(GLOB TEST_FILES "${CMAKE_CURRENT_SOURCE_DIR}/test*.cpp")

add_executable(supercontract-tests ${TEST_FILES} TestUtils.cpp ContractEnvironmentMock.cpp ContractEnvironmentMock.h ExecutorEnvironmentMock.cpp ExecutorEnvironmentMock.h VirtualMachineMock.cpp VirtualMachineMock.h StorageMock.cpp StorageMock.h)
enable_testing()
add_test(supercontract-tests "./test")
target_include_directories(supercontract-tests PRIVATE ../../src)
target_link_libraries(supercontract-tests supercontract contract-crypto Threads::Threads ${GTEST_LIBRARIES} gtest_main)