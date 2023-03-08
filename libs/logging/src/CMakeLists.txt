cmake_minimum_required(VERSION 3.9)
supercontract_sdk_shared_library(contract-logging)

supercontract_sdk_third_party_lib(spdlog.cmake)

target_include_directories(contract-logging PUBLIC ../include)
target_link_libraries(contract-logging PUBLIC spdlog)