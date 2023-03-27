cmake_minimum_required(VERSION 3.9)

set(CMAKE_CXX_STANDARD 20)

find_package(GTest REQUIRED)

file(GLOB TEST_FILES "${CMAKE_CURRENT_SOURCE_DIR}/test*.cpp")

if (SIRIUS_CONTRACT_MESSENGER_ECHO_ADDRESS_TEST)
    add_definitions(-DSIRIUS_CONTRACT_MESSENGER_ECHO_ADDRESS_TEST="${SIRIUS_CONTRACT_MESSENGER_ECHO_ADDRESS_TEST}")
endif ()

add_executable(sirius.contract.messenger.tests ${TEST_FILES} TestUtils.cpp)
supercontract_sdk_add_test(sirius.contract.messenger.tests)
target_include_directories(sirius.contract.messenger.tests PUBLIC ../src)
target_link_libraries(sirius.contract.messenger.tests sirius.contract.messenger Threads::Threads ${GTEST_LIBRARIES} gtest_main)

