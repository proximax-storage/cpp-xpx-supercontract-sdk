cmake_minimum_required(VERSION 3.9)

supercontract_sdk_shared_library_target(contract-common)
target_include_directories(contract-common PUBLIC ../include)
