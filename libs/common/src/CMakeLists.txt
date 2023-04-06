cmake_minimum_required(VERSION 3.9)

supercontract_sdk_library_target(sirius.contract.common ../include/common)
target_include_directories(sirius.contract.common PUBLIC ../include)
target_link_libraries(sirius.contract.common expected sirius.contract.logging sirius.contract.utils)
