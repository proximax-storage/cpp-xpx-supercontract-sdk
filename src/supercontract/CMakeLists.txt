#cmake_minimum_required(VERSION 3.9)
set(CMAKE_CXX_STANDARD 20)

supercontract_sdk_shared_library_target(supercontract)

foreach(_service supercontract_server internet)

    # Proto file
    get_filename_component(${_service}_proto "../protos/${_service}.proto" ABSOLUTE)
    get_filename_component(${_service}_proto_path "${${_service}_proto}" PATH)

    # Generated sources
    set(${_service}_proto_srcs "${CMAKE_CURRENT_BINARY_DIR}/${_service}.pb.cc")
    set(${_service}_proto_hdrs "${CMAKE_CURRENT_BINARY_DIR}/${_service}.pb.h")
    set(${_service}_grpc_srcs "${CMAKE_CURRENT_BINARY_DIR}/${_service}.grpc.pb.cc")
    set(${_service}_grpc_hdrs "${CMAKE_CURRENT_BINARY_DIR}/${_service}.grpc.pb.h")
    add_custom_command(
            OUTPUT "${${_service}_proto_srcs}" "${${_service}_proto_hdrs}" "${${_service}_grpc_srcs}" "${${_service}_grpc_hdrs}"
            COMMAND ${_PROTOBUF_PROTOC}
            ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
            --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
            -I "${${_service}_proto_path}"
            --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
            "${${_service}_proto}"
            DEPENDS "${${_service}_proto}")

    # Include generated *.pb.h files
    include_directories("${CMAKE_CURRENT_BINARY_DIR}")

    # vm_client_grpc_proto
    add_library(${_service}_grpc_proto
            ${${_service}_grpc_srcs}
            ${${_service}_grpc_hdrs}
            ${${_service}_proto_srcs}
            ${${_service}_proto_hdrs})
    target_link_libraries(${_service}_grpc_proto
            ${_REFLECTION}
            ${_GRPC_GRPCPP}
            ${_PROTOBUF_LIBPROTOBUF})

    # Targets greeter_[async_](client|server)
    target_link_libraries(supercontract
            ${_service}_grpc_proto
            ${_REFLECTION}
            ${_GRPC_GRPCPP}
            ${_PROTOBUF_LIBPROTOBUF})
endforeach()

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    add_definitions(-D_GNU_SOURCE)

    target_link_libraries(supercontract contract-crypto)
#            "-framework CoreFoundation" "-framework SystemConfiguration")
else()
	target_link_libraries(supercontract contract-crypto grpc++)
endif()
