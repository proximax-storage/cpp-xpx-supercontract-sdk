### set general cmake settings
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

### set boost settings
add_definitions(-DBOOST_ALL_DYN_LINK)
set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)

### set compiler settings
if(MSVC)
        # TODO This set of settings has not be checked for correctness
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /EHsc /DBOOST_LOG_DYN_LINK")
        # in debug disable "potentially uninitialized local variable" (FP)
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MDd /D_SCL_SECURE_NO_WARNINGS /wd4701")
        set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /MD /Zi")
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MD")

        set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /DEBUG:FASTLINK")
        set(CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO "${CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO} /DEBUG")

        add_definitions(-D_WIN32_WINNT=0x0601 /w44287 /w44388)

        # explicitly disable linking against static boost libs
        add_definitions(-DBOOST_ALL_NO_LIB)

        # min/max macros are useless
        add_definitions(-DNOMINMAX)
        add_definitions(-DWIN32_LEAN_AND_MEAN)
        add_definitions(-D _WIN32_WINNT=0x0601)
elseif("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU" OR "${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
        # -Wstrict-aliasing=1 perform most paranoid strict aliasing checks
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-error -Wno-error=unused-but-set-variable -Wno-error=attributes -Wno-error=unused-variable -Wno-error=cpp -Wstrict-aliasing=1 -Wnon-virtual-dtor -Wno-error=uninitialized -Wno-error=unknown-pragmas -Wno-unused-parameter -Wno-error=redundant-move -DBOOST_LOG_DYN_LINK")
        add_definitions(-D_STDC_WANT_LIB_EXT1_=1)
        add_definitions(-D__STDC_WANT_LIB_EXT1__=1)
endif()

# set runpath for built binaries on linux
if (NOT WIN32)
        if(("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU") OR ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang" AND "${CMAKE_SYSTEM_NAME}" MATCHES "Linux"))
                file(MAKE_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/boost")
                set(CMAKE_SKIP_BUILD_RPATH FALSE)

                set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_RPATH}:$ORIGIN:$ORIGIN/../lib")
                set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
                set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

                # use rpath for executables
                # (executable rpath will be used for loading indirect libs, this is needed because boost libs do not set runpath)
                # use newer runpath for shared libs
                set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--enable-new-dtags")
                set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--disable-new-dtags")
        endif()


        if(ARCHITECTURE_NAME)
                set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=${ARCHITECTURE_NAME}")
                set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=${ARCHITECTURE_NAME}")
        endif()
endif()

### define target helper functions

# used to define a catapult target (library, executable) and automatically enables PCH for clang
function(supercontract_sdk_target TARGET_NAME)
        set_property(TARGET ${TARGET_NAME} PROPERTY CXX_STANDARD 20)

        # indicate boost as a dependency
        target_link_libraries(${TARGET_NAME} ${Boost_LIBRARIES} ${CMAKE_DL_LIBS})

        # copy boost shared libraries
        foreach(BOOST_COMPONENT ATOMIC SYSTEM DATE_TIME REGEX TIMER CHRONO LOG THREAD FILESYSTEM) # PROGRAM_OPTIONS STACKTRACE_BACKTRACE)
                if(MSVC)
                        # copy into ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$(Configuration)
                        string(REPLACE ".lib" ".dll" BOOSTDLLNAME ${Boost_${BOOST_COMPONENT}_LIBRARY_RELEASE})
                        add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
                                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                                "${BOOSTDLLNAME}" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$(Configuration)")
                elseif("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
                        # copy into ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/boost
                        set(BOOSTDLLNAME ${Boost_${BOOST_COMPONENT}_LIBRARY_RELEASE})
                        set(BOOSTVERSION "${Boost_MAJOR_VERSION}.${Boost_MINOR_VERSION}.${Boost_SUBMINOR_VERSION}")
                        get_filename_component(BOOSTFILENAME ${BOOSTDLLNAME} NAME)
                        add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
                                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                                "${BOOSTDLLNAME}" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/boost")
                endif()
        endforeach()
endfunction()

# finds all files comprising a target
function(supercontract_sdk_find_all_target_files TARGET_TYPE TARGET_NAME)
        if (CMAKE_VERBOSE_MAKEFILE)
                message("processing ${TARGET_TYPE} '${TARGET_NAME}'")
        endif()

        file(GLOB ${TARGET_NAME}_INCLUDE_SRC "*.h")
        file(GLOB ${TARGET_NAME}_SRC "*.cpp")

        set(CURRENT_FILES ${${TARGET_NAME}_INCLUDE_SRC} ${${TARGET_NAME}_SRC})
        SOURCE_GROUP("src" FILES ${CURRENT_FILES})
        set(TARGET_FILES ${CURRENT_FILES})

        # add any (optional) subdirectories
        foreach(arg ${ARGN})
                set(SUBDIR ${arg})
                if (CMAKE_VERBOSE_MAKEFILE)
                        message("+ processing subdirectory '${arg}'")
                endif()

                file(GLOB ${TARGET_NAME}_${SUBDIR}_INCLUDE_SRC "${SUBDIR}/*.h")
                file(GLOB ${TARGET_NAME}_${SUBDIR}_SRC "${SUBDIR}/*.cpp")

                set(CURRENT_FILES ${${TARGET_NAME}_${SUBDIR}_INCLUDE_SRC} ${${TARGET_NAME}_${SUBDIR}_SRC})
                SOURCE_GROUP("${SUBDIR}" FILES ${CURRENT_FILES})
                set(TARGET_FILES ${TARGET_FILES} ${CURRENT_FILES})
        endforeach()

        set(${TARGET_NAME}_FILES ${TARGET_FILES} PARENT_SCOPE)
endfunction()

# used to define a supercontract sdk object library
function(supercontract_sdk_object_library TARGET_NAME)
        add_library(${TARGET_NAME} OBJECT ${ARGN})
        set_property(TARGET ${TARGET_NAME} PROPERTY POSITION_INDEPENDENT_CODE ON)
        set_property(TARGET ${TARGET_NAME} PROPERTY CXX_STANDARD 20)
endfunction()

# used to define a catapult library, creating an appropriate source group and adding a library
function(supercontract_sdk_library TARGET_NAME)
        supercontract_sdk_find_all_target_files("lib" ${TARGET_NAME} ${ARGN})
        add_library(${TARGET_NAME} ${${TARGET_NAME}_FILES})
endfunction()

# combines supercontract_sdk_library and supercontract_sdk_target
function(supercontract_sdk_library_target TARGET_NAME)
        supercontract_sdk_library(${TARGET_NAME} ${ARGN})
        set_property(TARGET ${TARGET_NAME} PROPERTY POSITION_INDEPENDENT_CODE ON)
        supercontract_sdk_target(${TARGET_NAME})
endfunction()

# used to define a supercontract sdk shared library, creating an appropriate source group and adding a library
function(supercontract_sdk_shared_library TARGET_NAME)
        supercontract_sdk_find_all_target_files("shared lib" ${TARGET_NAME} ${ARGN})

        add_definitions(-DDLL_EXPORTS)

        if(MSVC)
                set_win_version_definitions(${TARGET_NAME} VFT_DLL)
        endif()

        if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
           add_library(${TARGET_NAME} ${${TARGET_NAME}_FILES} ${VERSION_RESOURCES})
        else()
            add_library(${TARGET_NAME} SHARED ${${TARGET_NAME}_FILES} ${VERSION_RESOURCES})
        endif()
endfunction()

# combines supercontract_sdk_shared_library and supercontract_sdk_target
function(supercontract_sdk_shared_library_target TARGET_NAME)
        supercontract_sdk_shared_library(${TARGET_NAME} ${ARGN})
        supercontract_sdk_target(${TARGET_NAME})
endfunction()

# used to define a catapult executable, creating an appropriate source group and adding an executable
function(supercontract_sdk_executable TARGET_NAME)
        supercontract_sdk_find_all_target_files("exe" ${TARGET_NAME} ${ARGN})

        if(MSVC)
                set_win_version_definitions(${TARGET_NAME} VFT_APP)
        endif()

        add_executable(${TARGET_NAME} ${${TARGET_NAME}_FILES} ${VERSION_RESOURCES})

        if(WIN32 AND MINGW)
                target_link_libraries(${TARGET_NAME} wsock32 ws2_32)
        endif()
endfunction()

# used to define a catapult header only target, creating an appropriate source group in order to allow VS to create an appropriate folder
function(supercontract_sdk_header_only_target TARGET_NAME)
        if(MSVC)
                supercontract_sdk_find_all_target_files("hdr" ${TARGET_NAME} ${ARGN})

                if (CMAKE_VERBOSE_MAKEFILE)
                        foreach(arg ${ARGN})
                                message("adding subdirectory '${arg}'")
                        endforeach()
                endif()

                # https://stackoverflow.com/questions/39887352/how-to-create-a-cmake-header-only-library-that-depends-on-external-header-files
                # target_sources doesn't work with interface libraries, but we can use custom_target (with empty action)
                add_custom_target(${TARGET_NAME} SOURCES ${${TARGET_NAME}_FILES})
        endif()
endfunction()

function(supercontract_sdk_proto SERVICE PROTO_PATH DEPENDENCIES)
        if (NOT NOT_BUILD_SIRIUS_${SERVICE})
                message(ERROR built ${SERVICE}_sirius_grpc_proto)
                get_filename_component(${SERVICE}_proto "${PROTO_PATH}${SERVICE}.proto" ABSOLUTE)
                get_filename_component(${SERVICE}_proto_path "${${SERVICE}_proto}" PATH)
                list(APPEND DEPENDENCIES ${${SERVICE}_proto})

                # Generated sources
                set(${SERVICE}_proto_srcs "${CMAKE_CURRENT_BINARY_DIR}/${SERVICE}.pb.cc")
                set(${SERVICE}_proto_hdrs "${CMAKE_CURRENT_BINARY_DIR}/${SERVICE}.pb.h")
                set(${SERVICE}_grpc_srcs "${CMAKE_CURRENT_BINARY_DIR}/${SERVICE}.grpc.pb.cc")
                set(${SERVICE}_grpc_hdrs "${CMAKE_CURRENT_BINARY_DIR}/${SERVICE}.grpc.pb.h")
                add_custom_command(
                        OUTPUT "${${SERVICE}_proto_srcs}" "${${SERVICE}_proto_hdrs}" "${${SERVICE}_grpc_srcs}" "${${SERVICE}_grpc_hdrs}"
                        COMMAND ${_PROTOBUF_PROTOC}
                        ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
                        --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
                        -I "${${SERVICE}_proto_path}"
                        --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
                        "${${SERVICE}_proto}"
                        DEPENDS ${DEPENDENCIES})

                # vm_client_grpc_proto
                add_library(${SERVICE}_sirius_grpc_proto SHARED
                        ${${SERVICE}_grpc_srcs}
                        ${${SERVICE}_grpc_hdrs}
                        ${${SERVICE}_proto_srcs}
                        ${${SERVICE}_proto_hdrs})
                # Include generated *.pb.h files
                target_include_directories(${SERVICE}_sirius_grpc_proto PUBLIC "${CMAKE_CURRENT_BINARY_DIR}")
                target_link_libraries(${SERVICE}_sirius_grpc_proto
                        ${_REFLECTION}
                        ${_GRPC_GRPCPP}
                        ${_PROTOBUF_LIBPROTOBUF})
        endif()
endfunction()

function(supercontract_sdk_third_party_lib config)
        if("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU" OR "${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
                set(CMAKE_CXX_FLAGS "")
                set(CMAKE_C_FLAGS "")
        endif()
        include(${config})
endfunction()

function (supercontract_sdk_add_test executable)
        add_test(NAME ${executable}
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                COMMAND ${executable})
endfunction()