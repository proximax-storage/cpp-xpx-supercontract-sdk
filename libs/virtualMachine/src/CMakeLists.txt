cmake_minimum_required(VERSION 3.9)

find_package(OpenSSL REQUIRED)

supercontract_sdk_library_target(sirius.contract.vm)

foreach(service blockchain_vm internet_vm storage_vm)
    supercontract_sdk_proto(${service} "../protobuf/" "")
    target_link_libraries(sirius.contract.vm ${service}_contract_grpc_proto)
    list(APPEND SIRIUS_SUPERCONTRACT_DEPENDENCIES ${service}_contract_grpc_proto)
endforeach()

supercontract_sdk_proto(supercontract_server "../protobuf/" ${SIRIUS_SUPERCONTRACT_DEPENDENCIES})
target_link_libraries(sirius.contract.vm supercontract_server_contract_grpc_proto)

target_include_directories(sirius.contract.vm PUBLIC ../include ../../../include)
target_include_directories(sirius.contract.vm PUBLIC SYSTEM ${Boost_INCLUDE_DIR})

target_link_libraries(sirius.contract.vm expected sirius.contract.common OpenSSL::SSL sirius.contract.storage sirius.contract.internet sirius.contract.blockchain)
