cmake_minimum_required(VERSION 3.9)

find_package(OpenSSL REQUIRED)

supercontract_sdk_shared_library_target(contract-virtualMachine)

foreach(service blockchain_vm internet_vm storage_vm)
    supercontract_sdk_proto(${service} "")
    target_link_libraries(contract-virtualMachine ${service}_contract_grpc_proto)
    list(APPEND SIRIUS_SUPERCONTRACT_DEPENDENCIES ${service}_contract_grpc_proto)
endforeach()

supercontract_sdk_proto(supercontract_server ${SIRIUS_SUPERCONTRACT_DEPENDENCIES})
target_link_libraries(contract-virtualMachine supercontract_server_contract_grpc_proto)

target_include_directories(contract-virtualMachine PUBLIC ../include ../../../include)
target_include_directories(contract-virtualMachine PUBLIC SYSTEM ${Boost_INCLUDE_DIR})

target_link_libraries(contract-virtualMachine expected contract-logging contract-utils OpenSSL::SSL contract-storage contract-internet contract-blockchain)
