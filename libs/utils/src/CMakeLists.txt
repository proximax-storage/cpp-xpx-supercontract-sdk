cmake_minimum_required(VERSION 3.9)

supercontract_sdk_library_target(utils traits)
target_include_directories(utils PUBLIC ../include)
target_link_libraries(utils cereal::cereal)
set_property(TARGET utils PROPERTY CXX_STANDARD 17)
