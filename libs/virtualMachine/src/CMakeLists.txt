cmake_minimum_required(VERSION 3.9)

find_package(OpenSSL REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GRPC REQUIRED grpc++)
find_package(Protobuf REQUIRED)
find_program(_GRPC_CPP_PLUGIN_EXECUTABLE grpc_cpp_plugin PATHS /usr/bin /usr/local/bin)
find_program(_PROTOBUF_PROTOC protoc)

supercontract_sdk_library_target(sirius.contract.vm)

foreach(service blockchain_vm internet_vm storage_vm)
    supercontract_sdk_proto(${service} ${CMAKE_CURRENT_SOURCE_DIR} "")
    target_link_libraries(sirius.contract.vm ${service}_sirius_grpc_proto)
    list(APPEND SIRIUS_SUPERCONTRACT_DEPENDENCIES ${service}_sirius_grpc_proto)
endforeach()

supercontract_sdk_proto(supercontract_server ${CMAKE_CURRENT_SOURCE_DIR} "${SIRIUS_SUPERCONTRACT_DEPENDENCIES}")
target_link_libraries(sirius.contract.vm supercontract_server_sirius_grpc_proto)

target_include_directories(sirius.contract.vm PUBLIC ../include)
target_include_directories(sirius.contract.vm PUBLIC SYSTEM ${Boost_INCLUDE_DIR})

target_link_libraries(sirius.contract.vm expected sirius.contract.common OpenSSL::SSL sirius.contract.storage sirius.contract.internet sirius.contract.blockchain)

# Needed for proto linkage
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=default")
set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=default")
