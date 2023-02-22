cmake_minimum_required(VERSION 3.9)

find_package(OpenSSL REQUIRED)

supercontract_sdk_shared_library_target(contract-blockchain)

target_include_directories(contract-blockchain PUBLIC ../include ../../../include)

target_link_libraries(contract-blockchain expected contract-logging contract-utils contract-crypto OpenSSL::SSL)
