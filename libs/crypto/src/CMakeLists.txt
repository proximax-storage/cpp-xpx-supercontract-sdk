cmake_minimum_required(VERSION 3.9)

supercontract_sdk_library_target(sirius.contract.crypto)
target_include_directories(sirius.contract.crypto PUBLIC ../include)
target_link_libraries(sirius.contract.crypto 
			sirius.contract.utils 
			sirius.contract.crypto.external
			${Boost_STACKTRACE_BACKTRACE_LIBRARY}
)
