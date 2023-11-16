include(FetchContent)
add_definitions(-w)
FetchContent_Declare(
        cereal
        GIT_REPOSITORY https://github.com/USCiLab/cereal.git
        GIT_TAG        v1.3.2)
FetchContent_MakeAvailable(cereal)