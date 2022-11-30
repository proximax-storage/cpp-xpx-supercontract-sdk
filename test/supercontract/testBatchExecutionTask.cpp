/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "gtest/gtest.h"
#include "supercontract/BatchExecutionTask.h"
#include "ContractEnvironmentMock.h"
#include "VirtualMachineMock.h"
#include "ExecutorEnvironmentMock.h"
#include "utils/Random.h"
#include "StorageMock.h"

namespace sirius::contract::test {
#define TEST_NAME BatchExecutionTaskTest

TEST(TEST_NAME, FlowTest) {
    // create executor environment
    crypto::PrivateKey privateKey;
    crypto::KeyPair keyPair = crypto::KeyPair::FromPrivate(std::move(privateKey));
    ThreadManager threadManager;
    std::deque<bool> result = {true, true, false, false};
    auto virtualMachineMock = std::make_shared<VirtualMachineMock>(threadManager, result);
    std::weak_ptr<VirtualMachineMock> pVirtualMachineMock = virtualMachineMock;
    ExecutorConfig executorConfig;
    auto storageMock = std::make_shared<StorageMock>();
    std::weak_ptr<StorageMock> pStorageMock = storageMock;

    ExecutorEnvironmentMock executorEnvironmentMock(std::move(keyPair), pVirtualMachineMock, executorConfig,
                                                    threadManager, pStorageMock);
    // create batch
    std::deque<vm::CallRequest> callRequests;
    for(auto i=1; i<=1; i++){
        std::vector<uint8_t> params;
        CallRequestParameters requestParams{
                utils::generateRandomByteValue<ContractKey>(),
                utils::generateRandomByteValue<CallId>(),
                "",
                "",
                params,
                52000000,
                20 * 1024,
                CallReferenceInfo{
                        {},
                        0,
                        utils::generateRandomByteValue<BlockHash>(),
                        0,
                        0,
                        {}
                }
        };
        vm::CallRequest request(requestParams, vm::CallRequest::CallLevel::MANUAL);
        callRequests.push_back(request);
    }

    Batch batch = {
            1,
            callRequests
    };
    // create contract environment mock
    ContractKey contractKey;
    uint64_t automaticExecutionsSCLimit = 0;
    uint64_t automaticExecutionsSMLimit = 0;
    ContractEnvironmentMock contractEnvironmentMock(executorEnvironmentMock, contractKey, automaticExecutionsSCLimit,
                                                    automaticExecutionsSMLimit);


    // create batch execution task
    std::map<ExecutorKey, EndBatchExecutionOpinion> otherSuccessfulExecutorEndBatchOpinions;
    std::map<ExecutorKey, EndBatchExecutionOpinion> otherUnsuccessfulExecutorEndBatchOpinions;
    std::unique_ptr<BatchExecutionTask> pBatchExecutionTask;

    std::promise<void> barrier;
    threadManager.execute([&]{
        pBatchExecutionTask = std::make_unique<BatchExecutionTask>(std::move(batch),
                                                                  contractEnvironmentMock,
                                                                  executorEnvironmentMock,
                                                                  std::move(otherSuccessfulExecutorEndBatchOpinions),
                                                                  std::move(otherUnsuccessfulExecutorEndBatchOpinions),
                                                                  std::nullopt);
        pBatchExecutionTask->run();
        barrier.set_value();
    });
    sleep(3);
    std::cout<<otherSuccessfulExecutorEndBatchOpinions.size()<<std::endl;
    std::cout<<otherUnsuccessfulExecutorEndBatchOpinions.size()<<std::endl;
//    for(auto const &pair: otherSuccessfulExecutorEndBatchOpinions){
//        std::cout << "{" << pair.second.m_batchIndex << ": " << pair.second.isSuccessful() << "}\n";
//    }for(auto const &pair: otherUnsuccessfulExecutorEndBatchOpinions){
//        std::cout << "{" << pair.second.m_batchIndex << ": " << pair.second.isSuccessful() << "}\n";
//    }
    barrier.get_future().wait();

    threadManager.execute([&] {
        pBatchExecutionTask.reset();
    });

    threadManager.stop();
}
}