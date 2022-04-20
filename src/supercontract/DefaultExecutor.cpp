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

#include "utils/Serializer.h"

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

    ~DefaultExecutor() override {
        m_threadManager.execute([this] {
            for (auto& [_, contract] : m_contracts) {
                contract->terminate();
            }
        });

        m_threadManager.stop();
    }

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

            m_contracts[key] = createDefaultContract( key, std::move( request ), *this, m_config );

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

public:

    // region messenger event handler

    bool onMessageReceived( const std::string& tag, const std::string& msg ) override {

        try {
            if ( tag == "end_batch" ) {
                auto info = deserialize<EndBatchExecutionTransactionInfo>(msg);
                onEndBatchExecutionOpinionReceived( info );
                return true;
            }
        } catch(...)
        {
            _LOG_WARN( "onMessageReceived: invalid message format: query=" << tag );
        }
        return false;
    }

    // endregion

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

    void onEndBatchExecutionOpinionReceived( const EndBatchExecutionTransactionInfo& info ) {
        auto contractIt = m_contracts.find(info.m_contractKey);
        if ( contractIt == m_contracts.end() ) {
            contractIt->second->onEndBatchExecutionOpinionReceived( info );
        }
    }

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