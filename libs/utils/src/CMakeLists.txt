cmake_minimum_required(VERSION 3.9)

supercontract_sdk_library_target(contract-utils traits)
target_include_directories(contract-utils PUBLIC ../include)
target_link_libraries(contract-utils cereal::cereal)
set_property(TARGET contract-utils PROPERTY CXX_STANDARD 17)
