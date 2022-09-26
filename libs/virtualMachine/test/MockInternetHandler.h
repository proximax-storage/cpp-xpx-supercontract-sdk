#include <virtualMachine/VirtualMachineInternetQueryHandler.h>

namespace sirius::contract::vm::test
{
    class MockVirtualMachineInternetQueryHandler : public VirtualMachineInternetQueryHandler
    {
    public:
        void openConnection(
            const std::string &url,
            std::shared_ptr<AsyncQueryCallback<uint64_t>> callback);

        void read(
            uint64_t connectionId,
            std::shared_ptr<AsyncQueryCallback<std::vector<uint8_t>>> callback);

        void closeConnection(
            uint64_t connectionId,
            std::shared_ptr<AsyncQueryCallback<void>> callback);

        ~MockVirtualMachineInternetQueryHandler() = default;
    };
}