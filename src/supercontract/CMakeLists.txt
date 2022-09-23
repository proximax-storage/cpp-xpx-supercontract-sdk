cmake_minimum_required(VERSION 3.9)
set(CMAKE_CXX_STANDARD 20)

supercontract_sdk_shared_library_target(supercontract)

target_include_directories(supercontract PUBLIC ../../include/)
target_include_directories(supercontract PUBLIC SYSTEM ${Boost_INCLUDE_DIR})
target_link_libraries(supercontract
        expected
        cereal::cereal
        contract-internet
        contract-virtualMachine
        contract-crypto)
