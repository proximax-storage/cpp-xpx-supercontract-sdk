cmake_minimum_required(VERSION 3.9)

supercontract_sdk_executable(sirius.contract.executor_service)

supercontract_sdk_proto(executor "")

target_link_libraries(sirius.contract.executor_service sirius.contract.executor executor_contract_grpc_proto)
