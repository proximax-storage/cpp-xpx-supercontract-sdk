cmake_minimum_required(VERSION 3.9)

find_package(OpenSSL REQUIRED)

supercontract_sdk_library_target(contract-messenger)

supercontract_sdk_proto(messengerServer "")
target_link_libraries(contract-messenger messengerServer_contract_grpc_proto)

target_include_directories(contract-messenger PUBLIC ../include ../../../include)
target_include_directories(contract-messenger PUBLIC SYSTEM ${Boost_INCLUDE_DIR})

target_link_libraries(contract-messenger expected contract-logging contract-utils OpenSSL::SSL)
