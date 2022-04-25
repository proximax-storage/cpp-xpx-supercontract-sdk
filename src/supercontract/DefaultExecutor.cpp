/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "Contract.h"
#include "ThreadManager.h"
#include "ExecutorEnvironment.h"
#include "Messages.h"
#include "supercontract/eventHandlers/VirtualMachineEventHandler.h"

#include "contract/Executor.h"
#include "crypto/KeyPair.h"
#include "types.h"

#include "utils/Serializer.h"
#include "log.h"

namespace sirius::contract {

class DefaultExecutor
        : public Executor,
          public VirtualMachineEventHandler,
          public ExecutorEnvironment {

private:
    const crypto::KeyPair& m_keyPair;

    std::map<ContractKey, std::unique_ptr<Contract>>    m_contracts;
    std::map<DriveKey, ContractKey>                     m_contractsDriveKeys;

    ThreadManager m_threadManager;
    const ExecutorConfig m_config;

    ExecutorEventHandler& m_eventHandler;
    Messenger& m_messenger;
    Storage& m_storage;

    std::unique_ptr<VirtualMachine> m_virtualMachine;

    std::string m_dbgPeerName;

public:

    DefaultExecutor( const crypto::KeyPair& keyPair,
                     const ExecutorConfig config,
                     ExecutorEventHandler& eventHandler,
                     Messenger& messenger,
                     Storage& storage,
                     const std::string& dbgPeerName = "executor")
                     : m_keyPair(keyPair)
                     , m_config( config )
                     , m_eventHandler( eventHandler )
                     , m_messenger( messenger )
                     , m_storage( storage )
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
                _LOG_ERR( "The Executor Is Not In List" )
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
                _LOG_ERR( "Add Call to Non-Existing Contract " << key );
                return;
            }

            contractIt->second->addContractCall( request );
        } );
    }

    void removeContract( const ContractKey& key, RemoveRequest&& request ) override {
        m_threadManager.execute( [=, this, request = std::move( request )] {

            auto contractIt = m_contracts.find( key );

            if ( contractIt == m_contracts.end()) {
                _LOG_ERR( "Remove Non-Existing Contract " << key );
                return;
            }

            contractIt->second->removeContract( request );
        } );
    }

    void setExecutors( const ContractKey& key, std::set<ExecutorKey>&& executors ) override {
        m_threadManager.execute([=, this, executors=std::move(executors)] () mutable {

            auto contractIt = m_contracts.find(key);

            auto executorIt = executors.find(m_keyPair.publicKey());

            if ( executorIt == executors.end() ) {
                _LOG_ERR("The Executor Is Not In List")
                return;
            }

            executors.erase(executorIt);

            contractIt->second->setExecutors(std::move(executors));
        });
    }

public:

    // region storage event handler

    void onInitiatedModifications( const DriveKey& driveKey, uint64_t batchIndex ) override {
        m_threadManager.execute([=, this] {
            auto driveIt = m_contractsDriveKeys.find(driveKey);

            if ( driveIt == m_contractsDriveKeys.end()) {
                return;
            }

            auto contractIt = m_contracts.find(driveIt->second);

            if ( contractIt == m_contracts.end() ) {

                // TODO maybe error?
                return;
            }

            contractIt->second->onInitiatedModifications(batchIndex);
        });
    }

    void onAppliedSandboxStorageModifications( const DriveKey& driveKey, uint64_t batchIndex, bool success,
                                               int64_t sandboxSizeDelta, int64_t stateSizeDelta ) override {
        m_threadManager.execute([=, this] {
            auto driveIt = m_contractsDriveKeys.find(driveKey);

            if ( driveIt == m_contractsDriveKeys.end()) {
                return;
            }

            auto contractIt = m_contracts.find(driveIt->second);

            if ( contractIt == m_contracts.end() ) {

                // TODO maybe error?
                return;
            }

            contractIt->second->onAppliedSandboxStorageModifications(batchIndex, success, sandboxSizeDelta, stateSizeDelta);
        });
    }

    void
    onStorageHashEvaluated( const DriveKey& driveKey, uint64_t batchIndex, const StorageHash& storageHash, uint64_t usedDriveSize,
                            uint64_t metaFilesSize, uint64_t fileStructureSize ) override {
        m_threadManager.execute([=, this] {
            auto driveIt = m_contractsDriveKeys.find(driveKey);

            if ( driveIt == m_contractsDriveKeys.end()) {
                return;
            }

            auto contractIt = m_contracts.find(driveIt->second);

            if ( contractIt == m_contracts.end() ) {

                // TODO maybe error?
                return;
            }

            contractIt->second->onStorageHashEvaluated( batchIndex, storageHash, usedDriveSize, metaFilesSize,
                                                        fileStructureSize );
        });
    }

    // endregion

public:

    // region virtual machine event handler

    void
    onSuperContractCallExecuted( const ContractKey& contractKey, const CallExecutionResult& executionResult ) override {
        m_threadManager.execute( [=, this] {

            if ( auto it = m_contracts.find( contractKey ); it != m_contracts.end()) {
                it->second->onSuperContractCallExecuted( executionResult );
            }
        } );
    }

    // endregion

public:

    // region message event handler

    void onMessageReceived( const std::string& tag, const std::string& msg ) override {
        m_threadManager.execute( [=, this] {
            try {
                if ( tag == "end_batch" ) {
                    auto info = utils::deserialize<EndBatchExecutionOpinion>( msg );
                    onEndBatchExecutionOpinionReceived( info );
                    return true;
                }
            } catch (...) {
                _LOG_WARN( "onMessageReceived: invalid message format: query=" << tag );
            }
            return false;
        } );
    }

    // endregion

public:

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

    Storage& storage() override {
        return m_storage;
    }

    ExecutorEventHandler& executorEventHandler() override {
        return m_eventHandler;
    }

    VirtualMachine& virtualMachine() override {
        return *m_virtualMachine;
    }

    std::string dbgPeerName() override {
        return m_dbgPeerName;
    }

    // endregion

public:

    // region blockchain event handler

    void onEndBatchExecutionPublished( PublishedEndBatchExecutionTransactionInfo&& info ) override {
        m_threadManager.execute([this, info = std::move(info)] {
            auto contractIt = m_contracts.find(info.m_contractKey);

            if ( contractIt == m_contracts.end() ) {

                // TODO maybe error?
                return;
            }

            contractIt->second->onEndBatchExecutionPublished(info);
        });
    }

    void onEndBatchExecutionSingleTransactionPublished(
            PublishedEndBatchExecutionSingleTransactionInfo&& info ) override {
        m_threadManager.execute([this, info = std::move(info)] {
            auto contractIt = m_contracts.find(info.m_contractKey);

            if ( contractIt == m_contracts.end() ) {

                // TODO maybe error?
                return;
            }

            contractIt->second->onEndBatchExecutionSingleTransactionPublished(info);
        });
    }

    void onEndBatchExecutionFailed( FailedEndBatchExecutionTransactionInfo&& info ) override {
        m_threadManager.execute([this, info = std::move(info)] {
            auto contractIt = m_contracts.find(info.m_contractKey);

            contractIt->second->onEndBatchExecutionFailed(info);
        });
    }

    void onStorageSynchronized( const ContractKey& contractKey, uint64_t batchIndex ) override {
        m_threadManager.execute([=, this] {
            auto contractIt = m_contracts.find(contractKey);

            if ( contractIt == m_contracts.end() ) {

                // TODO maybe error?
                return;
            }

            contractIt->second->onStorageSynchronized(batchIndex);
        });
    }

    // endregion

private:

    void onEndBatchExecutionOpinionReceived( const EndBatchExecutionOpinion& opinion ) {
        if (!opinion.hasValidForm() || !opinion.verify()) {
            return;
        }

        auto contractIt = m_contracts.find(opinion.m_contractKey);
        if ( contractIt == m_contracts.end() ) {
            contractIt->second->onEndBatchExecutionOpinionReceived( opinion );
        }
    }
};

}