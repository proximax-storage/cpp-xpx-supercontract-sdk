cmake_minimum_required(VERSION 3.9)

find_package(OpenSSL REQUIRED)

supercontract_sdk_library_target(sirius.contract.messenger)

supercontract_sdk_proto(messengerServer "../protobuf/" "")
target_link_libraries(sirius.contract.messenger messengerServer_contract_grpc_proto)

target_include_directories(sirius.contract.messenger PUBLIC ../include ../../../include)
target_include_directories(sirius.contract.messenger PUBLIC SYSTEM ${Boost_INCLUDE_DIR})

target_link_libraries(sirius.contract.messenger expected sirius.contract.common OpenSSL::SSL)
