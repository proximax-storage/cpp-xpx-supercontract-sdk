cmake_minimum_required(VERSION 3.9)
supercontract_sdk_shared_library(contract-logging)
FetchContent_Declare(
        spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG        v1.10.0)
FetchContent_MakeAvailable(spdlog)
target_compile_definitions(spdlog PUBLIC -DSPDLOG_COMPILED_LIB -DSPDLOG_SHARED_LIB)
target_include_directories(contract-logging PUBLIC ../include)
target_link_libraries(contract-logging PUBLIC spdlog)