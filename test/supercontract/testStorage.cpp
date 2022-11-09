#include "ContractEnvironmentMock.h"
#include "ExecutorEnvironmentMock.h"
#include "TestUtils.h"
#include "supercontract/CallExecutionEnvironment.h"
#include "virtualMachine/RPCVirtualMachineBuilder.h"
#include "virtualMachine/VirtualMachine.h"
#include <gtest/gtest.h>

namespace sirius::contract::test {
// https://stackoverflow.com/questions/478898/how-do-i-execute-a-command-and-get-the-output-of-the-command-within-c-using-po
std::string exec(const char* cmd) {
    std::array<char, 128> buffer{};
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    std::cout << "exec result " << result;
    return result;
}

TEST(VirtualMachine, SimpleStorage) {
    crypto::PrivateKey privateKey;
    crypto::KeyPair keyPair = crypto::KeyPair::FromPrivate(std::move(privateKey));
    ExecutorConfig executorConfig;
    ThreadManager threadManager;
    std::shared_ptr<vm::VirtualMachine> pVirtualMachine;
    ExecutorEnvironmentMock environment(std::move(keyPair), pVirtualMachine, executorConfig,
                                        threadManager);

    ContractKey contractKey;
    uint64_t automaticExecutionsSCLimit = 0;
    uint64_t automaticExecutionsSMLimit = 0;
    ContractEnvironmentMock contractEnvironmentMock(contractKey, automaticExecutionsSCLimit,
                                                    automaticExecutionsSMLimit);

    auto storageObserver = std::make_shared<StorageObserverMock>();

    std::shared_ptr<CallExecutionEnvironment> rpcHandler;

    std::promise<void> p;
    auto barrier = p.get_future();

    const auto copyOptions = std::filesystem::copy_options::overwrite_existing;
    threadManager.execute([&] {
        // exec("cp ../../libs/virtualMachine/test/supercontracts/simple.rs ../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/src/lib.rs");
        std::filesystem::copy("../../libs/virtualMachine/test/supercontracts/storage.rs",
                              "../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/src/lib.rs",
                              copyOptions);
        exec("wasm-pack build --debug ../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/");
        // TODO Fill in the address
        std::string address = "127.0.0.1:50051";
        vm::RPCVirtualMachineBuilder builder;
        pVirtualMachine = builder.build(storageObserver, environment, address);

        // TODO fill in the callRequest fields
        std::vector<uint8_t> params;
        vm::CallRequest callRequest = vm::CallRequest(CallRequestParameters{
                                                          ContractKey(),
                                                          CallId(),
                                                          "../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/pkg/sdk_bg.wasm",
                                                          "run",
                                                          params,
                                                          25000000000,
                                                          26 * 1024,
                                                          CallReferenceInfo{
                                                              {},
                                                              0,
                                                              BlockHash(),
                                                              0,
                                                              0,
                                                              {}}},
                                                      vm::CallRequest::CallLevel::MANUAL);

        rpcHandler = std::make_shared<CallExecutionEnvironment>(callRequest, environment, contractEnvironmentMock);

        auto [_, callback] = createAsyncQuery<vm::CallExecutionResult>([&](auto&& res) {
            p.set_value();
            ASSERT_TRUE(res);
            ASSERT_EQ(res->m_success, true);
            ASSERT_EQ(res->m_return, 1);
            ASSERT_EQ(res->m_scConsumed, 4402851510);
            ASSERT_EQ(res->m_smConsumed, 56); }, [] {}, environment, false, false);

        pVirtualMachine->executeCall(callRequest, rpcHandler, rpcHandler, rpcHandler, callback);
    });

    barrier.get();

    threadManager.execute([&] { pVirtualMachine.reset(); });

    threadManager.stop();
    // exec("cp ../../libs/virtualMachine/test/supercontracts/lib.rs ../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/src/lib.rs");
    std::filesystem::copy("../../libs/virtualMachine/test/supercontracts/lib.rs",
                          "../../libs/virtualMachine/test/rust-xpx-supercontract-client-sdk/src/lib.rs", copyOptions);
}
} // namespace sirius::contract::test