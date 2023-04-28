cmake_minimum_required(VERSION 3.9)

find_package(OpenSSL REQUIRED)

supercontract_sdk_proto(blockchain "../protobuf/" "")

supercontract_sdk_library_target(sirius.contract.blockchain)

target_include_directories(sirius.contract.blockchain PUBLIC ../include)

target_link_libraries(sirius.contract.blockchain expected sirius.contract.common sirius.contract.crypto OpenSSL::SSL blockchain_sirius_grpc_proto)

# Needed for proto linkage
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=default")
set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=default")