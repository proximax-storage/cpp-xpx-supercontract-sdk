/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include <map>
#include <memory>

#include "types.h"

#include "log.h"
#include "DebugInfo.h"

namespace sirius::contract {

    template <class THandler>
    class VirtualMachineQueryHandlersKeeper {

    private:

        std::map<CallId, std::shared_ptr<THandler>> m_handlers;
        const DebugInfo m_dbgInfo;

    public:

        VirtualMachineQueryHandlersKeeper( const DebugInfo& debugInfo )
        : m_dbgInfo( debugInfo ) {
            DBG_MAIN_THREAD
        }

        ~VirtualMachineQueryHandlersKeeper() {

            DBG_MAIN_THREAD

            for (auto& [_, pHandler]: m_handlers) {
                pHandler->terminate();
            }
        }

        std::shared_ptr<THandler> getHandler( const CallId& callId ) const {

            DBG_MAIN_THREAD

            auto it = m_handlers.template find(callId);
            if (it == m_handlers.end()) {
                return {};
            }
            return it->second;
        }

        void addHandler( const CallId& callId, std::shared_ptr<THandler> handler ) {

            DBG_MAIN_THREAD

            m_handlers[callId] = std::move( handler );
        }

        void removeHandler( const CallId& callId ) {

            DBG_MAIN_THREAD

            auto it = m_handlers.template find(callId);
            m_handlers.erase(it);
        }
    };

}