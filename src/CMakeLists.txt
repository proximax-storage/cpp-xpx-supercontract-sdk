cmake_minimum_required(VERSION 3.9)

include_directories(${PROJECT_SOURCE_DIR}/include ${PROJECT_SOURCE_DIR}/src)

#add_subdirectory(proto)
add_subdirectory(utils)
add_subdirectory(external)
add_subdirectory(crypto)
add_subdirectory(supercontract)
