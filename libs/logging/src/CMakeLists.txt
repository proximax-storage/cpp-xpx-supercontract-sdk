cmake_minimum_required(VERSION 3.9)

supercontract_sdk_shared_library(contract-logging)
target_include_directories(contract-logging PUBLIC ../include)
target_link_libraries(contract-logging spdlog)
