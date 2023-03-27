cmake_minimum_required(VERSION 3.9)

find_package(GTest REQUIRED)

if (SIRIUS_CONTRACT_STORAGE_ADDRESS_TEST)
    add_definitions(-DSIRIUS_CONTRACT_STORAGE_ADDRESS_TEST="${SIRIUS_CONTRACT_STORAGE_ADDRESS_TEST}")
endif ()

if(SIRIUS_CONTRACT_VM_ADDRESS_TEST)
    add_definitions(-DSIRIUS_CONTRACT_VM_ADDRESS_TEST="${SIRIUS_CONTRACT_VM_ADDRESS_TEST}")
endif()

file(GLOB TEST_FILES "${CMAKE_CURRENT_SOURCE_DIR}/test*.cpp")

add_executable(sirius.contract.executor.tests ${TEST_FILES} TestUtils.cpp ContractEnvironmentMock.cpp ExecutorEnvironmentMock.cpp VirtualMachineMock.cpp StorageMock.cpp StorageUtils.cpp SandboxModificationMock.cpp StorageModificationMock.cpp ExecutorEventHandlerMock.cpp MessengerMock.cpp BatchesManagerMock.cpp)
supercontract_sdk_add_test(sirius.contract.executor.tests)
target_include_directories(sirius.contract.executor.tests PRIVATE ../src)
target_link_libraries(sirius.contract.executor.tests sirius.contract.executor sirius.contract.crypto Threads::Threads ${GTEST_LIBRARIES} gtest_main)