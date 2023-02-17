#include <virtualMachine/VirtualMachineInternetQueryHandler.h>

namespace sirius::contract::vm::test
{
    class MockVirtualMachineInternetQueryHandler : public VirtualMachineInternetQueryHandler
    {
        const int BUFFER_SIZE = 16 * 1024;

    private:
        std::string m_read_buffer;
        int m_read_pointer;

    public:
        MockVirtualMachineInternetQueryHandler();

        void openConnection(
            const std::string &url,
            bool softRevocationMode,
            std::shared_ptr<AsyncQueryCallback<uint64_t>> callback) override;

        void read(
            uint64_t connectionId,
            std::shared_ptr<AsyncQueryCallback<std::vector<uint8_t>>> callback) override;

        void closeConnection(
            uint64_t connectionId,
            std::shared_ptr<AsyncQueryCallback<void>> callback) override;
    };
}