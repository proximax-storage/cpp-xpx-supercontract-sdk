include(FetchContent)

FetchContent_Declare(
        expected
        GIT_REPOSITORY https://github.com/TartanLlama/expected
        GIT_TAG        v1.0.0)
FetchContent_MakeAvailable(expected)