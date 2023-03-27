cmake_minimum_required(VERSION 3.9)

supercontract_sdk_library_target(sirius.contract.utils traits)
target_include_directories(sirius.contract.utils PUBLIC ../include)
target_link_libraries(sirius.contract.utils cereal::cereal)
set_property(TARGET sirius.contract.utils PROPERTY CXX_STANDARD 17)
