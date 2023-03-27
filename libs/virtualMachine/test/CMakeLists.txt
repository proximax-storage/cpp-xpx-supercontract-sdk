cmake_minimum_required(VERSION 3.9)

find_package(GTest REQUIRED)

if(SIRIUS_CONTRACT_VM_ADDRESS_TEST)
    add_definitions(-DSIRIUS_CONTRACT_VM_ADDRESS_TEST="${SIRIUS_CONTRACT_VM_ADDRESS_TEST}")
endif()

if(SIRIUS_CONTRACT_RUN_SUDO_TESTS)
    add_definitions(-DSIRIUS_CONTRACT_RUN_SUDO_TESTS)
endif()

file(GLOB TEST_FILES "${CMAKE_CURRENT_SOURCE_DIR}/test*.cpp")

add_executable(sirius.contract.vm.tests ${TEST_FILES} TestUtils.cpp MockInternetHandler.cpp MockStorageHandler.cpp FaultyMockStorageHandler.cpp)
supercontract_sdk_add_test(sirius.contract.vm.tests)
target_link_libraries(sirius.contract.vm.tests sirius.contract.vm Threads::Threads ${GTEST_LIBRARIES} gtest_main)

