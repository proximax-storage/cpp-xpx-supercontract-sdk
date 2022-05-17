/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include <map>
#include <memory>

#include "types.h"

namespace sirius::contract {

    template <class THandler>
    class VirtualMachineQueryHandlersKeeper {

    private:

        std::map<CallId, std::weak_ptr<THandler>> m_handlers;

    public:

        std::shared_ptr<THandler> getHandler(const CallId& callId);

        void addHandler(const CallId& callId, const std::shared_ptr<THandler>&);

        void removeHandler(const CallId& callId);

    };

}