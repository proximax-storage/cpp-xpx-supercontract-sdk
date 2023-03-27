cmake_minimum_required(VERSION 3.9)

add_subdirectory(src)
if (SIRIUS_CONTRACT_BUILD_TESTS)
    add_subdirectory(test)
endif ()
