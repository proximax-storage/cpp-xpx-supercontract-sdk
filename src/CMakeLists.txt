cmake_minimum_required(VERSION 3.9)

# Needed for proto linkage
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=default")
set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=default")

supercontract_sdk_proto(executor "../protobuf/" "")

add_subdirectory(rpcExecutorClient)
add_subdirectory(rpcExecutorServer)

