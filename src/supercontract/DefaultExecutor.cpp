/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "Contract.h"
#include "ThreadManager.h"
#include "supercontract/eventHandlers/VirtualMachineEventHandler.h"

#include "contract/Executor.h"
#include "contract/StorageBridgeEventHandler.h"
#include "contract/MessengerEventHandler.h"
#include "crypto/KeyPair.h"
#include "types.h"

namespace sirius::contract {

class DefaultExecutor
        : public Executor,
          public VirtualMachineEventHandler,
          public StorageBridgeEventHandler,
          public MessengerEventHandler {

    void addContract( const ContractKey& key, AddContractRequest&& request ) override {
        m_threadManager.execute([=, this, request = std::move(request)] () mutable {
            if (m_contracts.contains(key)) {
                return;
            }

            auto it = request.m_executors.find(m_keyPair.publicKey());

            if (it == request.m_executors.end()) {
                // ERROR
                return;
            }

            request.m_executors.erase(it);

            m_contracts[key] = createDefaultContract(key, request, m_eventHandler, m_messenger, m_storageBridge);

        });
    }

    void addContractCall( const ContractKey& key, const CallRequest& request ) override {
        m_threadManager.execute([&, this] {

            auto contractIt = m_contracts.find(key);

            if (contractIt == m_contracts.end()) {
                return;
            }

            contractIt->second->addContractCall(request);
        });
    }

    void
    onSuperContractCallExecuted( const ContractKey& contractKey, const CallExecutionResult& executionResult ) override {
        if ( auto it = m_contracts.find( contractKey ); it != m_contracts.end()) {

        }
    }

public:
    bool onMessageReceived( const std::string& tag, const std::string& msg ) override {
        std::set<std::string> supportedTags = {};
        if (supportedTags.contains(tag)) {
            return true;
        }
        return false;
    }

private:
    const crypto::KeyPair& m_keyPair;
    std::map<ContractKey, std::unique_ptr<Contract>> m_contracts;
    ThreadManager m_threadManager;
    ExecutorEventHandler& m_eventHandler;
    Messenger& m_messenger;
    StorageBridge& m_storageBridge;
};

}