include(FetchContent)
add_definitions(-w)
FetchContent_Declare(
        magic_enum
        GIT_REPOSITORY https://github.com/Neargye/magic_enum
        GIT_TAG        v0.8.1)
FetchContent_MakeAvailable(magic_enum)