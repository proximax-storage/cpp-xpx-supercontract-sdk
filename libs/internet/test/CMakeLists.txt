cmake_minimum_required(VERSION 3.9)

find_package(GTest REQUIRED)

if(SIRIUS_CONTRACT_RUN_SUDO_TESTS)
    add_definitions(-DSIRIUS_CONTRACT_RUN_SUDO_TESTS)
endif()

file(GLOB TEST_FILES "${CMAKE_CURRENT_SOURCE_DIR}/test*.cpp")

add_executable(sirius.contract.internet.tests ${TEST_FILES})
supercontract_sdk_add_test(sirius.contract.internet.tests)
target_link_libraries(sirius.contract.internet.tests sirius.contract.internet Threads::Threads ${GTEST_LIBRARIES} gtest_main)

