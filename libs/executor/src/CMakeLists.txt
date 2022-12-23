cmake_minimum_required(VERSION 3.9)
set(CMAKE_CXX_STANDARD 20)

supercontract_sdk_shared_library_target(executor)

target_include_directories(executor PUBLIC ../include ../../../include)
target_include_directories(executor PUBLIC SYSTEM ${Boost_INCLUDE_DIR})
target_link_libraries(executor
        expected
        magic_enum
        cereal::cereal
        contract-internet
        contract-virtualMachine
        contract-crypto
        contract-messenger)
