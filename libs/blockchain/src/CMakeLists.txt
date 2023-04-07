cmake_minimum_required(VERSION 3.9)

find_package(OpenSSL REQUIRED)

supercontract_sdk_proto(blockchain "../protobuf/" "")

supercontract_sdk_library_target(sirius.contract.blockchain)

target_include_directories(sirius.contract.blockchain PUBLIC ../include)

target_link_libraries(sirius.contract.blockchain expected sirius.contract.common sirius.contract.crypto OpenSSL::SSL blockchain_contract_grpc_proto)