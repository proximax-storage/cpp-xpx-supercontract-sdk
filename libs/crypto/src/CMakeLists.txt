cmake_minimum_required(VERSION 3.9)

supercontract_sdk_library_target(contract-crypto)
target_include_directories(contract-crypto PUBLIC ../include)
target_link_libraries(contract-crypto utils sdk_external)
