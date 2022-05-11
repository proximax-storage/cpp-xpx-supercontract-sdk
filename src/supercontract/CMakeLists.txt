#cmake_minimum_required(VERSION 3.9)
set(CMAKE_CXX_STANDARD 20)

supercontract_sdk_shared_library_target(supercontract)

# Proto file
get_filename_component(vm_client_proto "../protos/supercontract_server.proto" ABSOLUTE)
get_filename_component(vm_client_proto_path "${vm_client_proto}" PATH)

# Generated sources
set(vm_client_proto_srcs "${CMAKE_CURRENT_BINARY_DIR}/supercontract_server.pb.cc")
set(vm_client_proto_hdrs "${CMAKE_CURRENT_BINARY_DIR}/supercontract_server.pb.h")
set(vm_client_grpc_srcs "${CMAKE_CURRENT_BINARY_DIR}/supercontract_server.grpc.pb.cc")
set(vm_client_grpc_hdrs "${CMAKE_CURRENT_BINARY_DIR}/supercontract_server.grpc.pb.h")
add_custom_command(
        OUTPUT "${vm_client_proto_srcs}" "${vm_client_proto_hdrs}" "${vm_client_grpc_srcs}" "${vm_client_grpc_hdrs}"
        COMMAND ${_PROTOBUF_PROTOC}
        ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
        --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
        -I "${vm_client_proto_path}"
        --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
        "${vm_client_proto}"
        DEPENDS "${vm_client_proto}")

# Include generated *.pb.h files
include_directories("${CMAKE_CURRENT_BINARY_DIR}")

# vm_client_grpc_proto
add_library(vm_client_grpc_proto
        ${vm_client_grpc_srcs}
        ${vm_client_grpc_hdrs}
        ${vm_client_proto_srcs}
        ${vm_client_proto_hdrs})
target_link_libraries(vm_client_grpc_proto
        ${_REFLECTION}
        ${_GRPC_GRPCPP}
        ${_PROTOBUF_LIBPROTOBUF})

message(ERROR "${vm_client_grpc_srcs}")

# Targets greeter_[async_](client|server)
target_link_libraries(supercontract
        vm_client_grpc_proto
        ${_REFLECTION}
        ${_GRPC_GRPCPP}
        ${_PROTOBUF_LIBPROTOBUF})

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    add_definitions(-D_GNU_SOURCE)

    target_link_libraries(supercontract contract-crypto)
#            "-framework CoreFoundation" "-framework SystemConfiguration")
else()
	target_link_libraries(supercontract contract-crypto grpc++)
endif()
