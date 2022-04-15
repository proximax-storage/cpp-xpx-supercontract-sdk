#cmake_minimum_required(VERSION 3.9)
add_definitions(-DSIRIUS_DRIVE_MULTI)
set(CMAKE_CXX_STANDARD 20)

supercontract_sdk_shared_library_target(supercontract)

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    add_definitions(-D_GNU_SOURCE)

    target_link_libraries(supercontract crypto)
#            "-framework CoreFoundation" "-framework SystemConfiguration")
else()
	target_link_libraries(supercontract crypto)
endif()
