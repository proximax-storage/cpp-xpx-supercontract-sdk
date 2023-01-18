cmake_minimum_required(VERSION 3.9)

project(supercontract-tests)

set(CMAKE_CXX_STANDARD 20)

find_package(GTest REQUIRED)

file(GLOB TEST_FILES "${CMAKE_CURRENT_SOURCE_DIR}/test*.cpp")

add_executable(executor-tests ${TEST_FILES} TestUtils.cpp ContractEnvironmentMock.cpp ExecutorEnvironmentMock.cpp VirtualMachineMock.cpp StorageMock.cpp ExecutorEventHandlerMock.cpp MessengerMock.cpp BatchesManagerMock.cpp)
enable_testing()
add_test(executor-tests "./test")
target_include_directories(executor-tests PRIVATE ../src)
target_link_libraries(executor-tests executor contract-crypto Threads::Threads ${GTEST_LIBRARIES} gtest_main)