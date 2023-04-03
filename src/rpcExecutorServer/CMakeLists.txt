cmake_minimum_required(VERSION 3.9)

supercontract_sdk_library_target(sirius.contract.rpc_executor_server)

target_include_directories(sirius.contract.rpc_executor_server PUBLIC "../../include")
target_link_libraries(sirius.contract.rpc_executor_server sirius.contract.executor executor_contract_grpc_proto)