/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "Contract.h"
#include "ThreadManager.h"
#include "ContractContext.h"
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
          public MessengerEventHandler,
          public ContractContext {

public:

    DefaultExecutor( const crypto::KeyPair& keyPair,
                     const ExecutorConfig config,
                     ExecutorEventHandler& eventHandler,
                     Messenger& messenger,
                     StorageBridge& storageBridge,
                     const std::string& dbgPeerName = "executor")
                     : m_keyPair(keyPair)
                     , m_config( config )
                     , m_eventHandler( eventHandler )
                     , m_messenger( messenger )
                     , m_storageBridge( storageBridge )
                     , m_dbgPeerName( dbgPeerName )
                     {}

    void addContract( const ContractKey& key, AddContractRequest&& request ) override {
        m_threadManager.execute( [=, this, request = std::move( request )]() mutable {
            if ( m_contracts.contains( key )) {
                return;
            }

            auto it = request.m_executors.find( m_keyPair.publicKey());

            if ( it == request.m_executors.end()) {
                // ERROR
                return;
            }

            request.m_executors.erase( it );

            m_contracts[key] = createDefaultContract( key, request, m_eventHandler, m_messenger, m_storageBridge );

        } );
    }

    void addContractCall( const ContractKey& key, CallRequest&& request ) override {
        m_threadManager.execute( [=, this, request = std::move( request )] {

            auto contractIt = m_contracts.find( key );

            if ( contractIt == m_contracts.end()) {
                return;
            }

            contractIt->second->addContractCall( request );
        } );
    }

    void
    onSuperContractCallExecuted( const ContractKey& contractKey, const CallExecutionResult& executionResult ) override {
        if ( auto it = m_contracts.find( contractKey ); it != m_contracts.end()) {

        }
    }

    bool onMessageReceived( const std::string& tag, const std::string& msg ) override {
        std::set<std::string> supportedTags = {};
        if (supportedTags.contains(tag)) {
            return true;
        }
        return false;
    }

    // region contract context

    const crypto::KeyPair& keyPair() const override {
        return m_keyPair;
    }

    ThreadManager& threadManager() override {
        return m_threadManager;
    }

    Messenger& messenger() override {
        return m_messenger;
    }

    StorageBridge& storageBridge() override {
        return m_storageBridge;
    }

    ExecutorEventHandler& executorEventHandler() override {
        return m_eventHandler;
    }

    std::string dbgPeerName() override {
        return m_dbgPeerName;
    }

    // endregion

private:
    const crypto::KeyPair& m_keyPair;

    std::map<ContractKey, std::unique_ptr<Contract>> m_contracts;
    std::map<DriveKey, ContractKey> m_contractsDriveKeys;

    ThreadManager m_threadManager;
    const ExecutorConfig m_config;

    ExecutorEventHandler& m_eventHandler;
    Messenger& m_messenger;
    StorageBridge& m_storageBridge;

    std::string m_dbgPeerName;
};

}