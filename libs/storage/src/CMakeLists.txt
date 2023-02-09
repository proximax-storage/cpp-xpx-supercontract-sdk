cmake_minimum_required(VERSION 3.9)

find_package(OpenSSL REQUIRED)

supercontract_sdk_shared_library_target(contract-storage)

supercontract_sdk_proto(storageServer "")
target_link_libraries(contract-storage storageServer_contract_grpc_proto)

target_include_directories(contract-storage PUBLIC ../include ../../../include)
target_include_directories(contract-storage PUBLIC SYSTEM ${Boost_INCLUDE_DIR})

target_link_libraries(contract-storage expected contract-logging contract-utils OpenSSL::SSL)
