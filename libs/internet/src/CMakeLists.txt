cmake_minimum_required(VERSION 3.9)

find_package(OpenSSL REQUIRED)

supercontract_sdk_library_target(contract-internet)
target_include_directories(contract-internet PUBLIC ../include ../../../include)
target_include_directories(contract-internet PUBLIC SYSTEM ${Boost_INCLUDE_DIR})

target_link_libraries(contract-internet expected contract-logging utils OpenSSL::SSL)
