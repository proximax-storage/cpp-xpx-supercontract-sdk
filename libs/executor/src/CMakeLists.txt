cmake_minimum_required(VERSION 3.9)

supercontract_sdk_library_target(sirius.contract.executor)

target_include_directories(sirius.contract.executor PUBLIC ../include)
target_include_directories(sirius.contract.executor PUBLIC SYSTEM ${Boost_INCLUDE_DIR})
target_link_libraries(sirius.contract.executor
        expected
        magic_enum
        cereal::cereal
        sirius.contract.internet
        sirius.contract.vm
        sirius.contract.crypto
        sirius.contract.messenger)
