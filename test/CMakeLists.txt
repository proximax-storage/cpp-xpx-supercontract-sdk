cmake_minimum_required(VERSION 3.2)
project(supercontract-tests)

set(CMAKE_CXX_STANDARD 20)

find_package(Threads REQUIRED)
find_package(Boost 1.71.0 REQUIRED)
find_package(GTest REQUIRED) # Find the google testing framework on your system

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    file(GLOB TEST_FILES "${CMAKE_CURRENT_SOURCE_DIR}/test*.cpp")
else()
    file(GLOB TEST_FILES "${CMAKE_CURRENT_SOURCE_DIR}/test*.cpp")
endif()

add_executable(supercontract-tests ${TEST_FILES})

include_directories(${GTEST_INCLUDE_DIRS})

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    add_definitions(-D_GNU_SOURCE)
    target_link_libraries(supercontract-tests
            supercontract
            contract-crypto
            ${GTEST_LIBRARIES}
            /usr/local/lib/libgtest_main.a
            )

    enable_testing()
    add_test(supercontract-tests "./test")

else()
    enable_testing()
    add_test(supercontract-tests "./test")
    target_link_libraries(supercontract-tests supercontract contract-crypto Threads::Threads ${GTEST_LIBRARIES} gtest_main)
endif()

