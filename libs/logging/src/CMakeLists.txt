cmake_minimum_required(VERSION 3.9)
supercontract_sdk_library_target(contract-logging)

supercontract_sdk_third_party_lib(spdlog.cmake)

target_include_directories(contract-logging PUBLIC ../include)
target_link_libraries(contract-logging spdlog)