cmake_minimum_required(VERSION 3.9)

find_package(OpenSSL REQUIRED)

supercontract_sdk_library_target(sirius.contract.storage)

supercontract_sdk_proto(storageServer "../protobuf/" "")
target_link_libraries(sirius.contract.storage storageServer_contract_grpc_proto)

target_include_directories(sirius.contract.storage PUBLIC ../include)
target_include_directories(sirius.contract.storage PUBLIC SYSTEM ${Boost_INCLUDE_DIR})

target_link_libraries(sirius.contract.storage expected sirius.contract.common OpenSSL::SSL)
