cmake_minimum_required(VERSION 3.9)

find_package(OpenSSL REQUIRED)

supercontract_sdk_library_target(sirius.contract.internet)
target_include_directories(sirius.contract.internet PUBLIC ../include ../../../include)
target_include_directories(sirius.contract.internet PUBLIC SYSTEM ${Boost_INCLUDE_DIR})

target_link_libraries(sirius.contract.internet expected sirius.contract.logging sirius.contract.utils OpenSSL::SSL)
