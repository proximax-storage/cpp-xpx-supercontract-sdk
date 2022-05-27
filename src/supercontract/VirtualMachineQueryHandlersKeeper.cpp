/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "VirtualMachineQueryHandlersKeeper.h"

namespace sirius::contract {
template<class THandler>
std::shared_ptr<THandler> contract::VirtualMachineQueryHandlersKeeper<THandler>::getHandler( const CallId& callId ) const {
    auto it = m_handlers.template find(callId);
    if (it == m_handlers.end()) {
        return {};
    }
    return it->second;
}

template<class THandler>
void
VirtualMachineQueryHandlersKeeper<THandler>::addHandler( const CallId& callId, std::shared_ptr<THandler> handler ) {
    m_handlers[callId] = std::move( handler );
}

template<class THandler>
void VirtualMachineQueryHandlersKeeper<THandler>::removeHandler( const CallId& callId ) {
    auto it = m_handlers.template find(callId);
    m_handlers.erase(it);
}

template<class THandler>
VirtualMachineQueryHandlersKeeper<THandler>::~VirtualMachineQueryHandlersKeeper() {
    for (auto& [_, pHandler]: m_handlers) {
        pHandler->terminate();
    }
}

}