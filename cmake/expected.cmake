option(EXPECTED_BUILD_TESTS OFF)
include(FetchContent)
add_definitions(-w)
FetchContent_Declare(
        expected
        GIT_REPOSITORY https://github.com/TartanLlama/expected
        GIT_TAG        v1.1.0)
FetchContent_MakeAvailable(expected)