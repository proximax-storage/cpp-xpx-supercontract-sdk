cmake_minimum_required(VERSION 3.9)

find_package(GTest REQUIRED)

if (SIRIUS_CONTRACT_STORAGE_ADDRESS_TEST)
    add_definitions(-DSIRIUS_CONTRACT_STORAGE_ADDRESS_TEST="${SIRIUS_CONTRACT_STORAGE_ADDRESS_TEST}")
endif ()

file(GLOB TEST_FILES "${CMAKE_CURRENT_SOURCE_DIR}/test*.cpp")

add_executable(sirius.contract.storage.tests ${TEST_FILES} TestUtils.cpp)
supercontract_sdk_add_test(sirius.contract.storage.tests)
target_include_directories(sirius.contract.storage.tests PUBLIC ../src)
target_link_libraries(sirius.contract.storage.tests sirius.contract.storage Threads::Threads ${GTEST_LIBRARIES} gtest_main)

