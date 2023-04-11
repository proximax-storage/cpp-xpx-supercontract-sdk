/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "ServerRPCTag.h"
#include <common/AsyncQuery.h>
#include <memory>

namespace sirius::contract::blockchain {

template<class TContext>
class FinishRequestTag : public ServerRPCTag {

public:

    std::unique_ptr<TContext> m_context;

public:

    FinishRequestTag(std::unique_ptr<TContext>&& context)
            : m_context(std::move(context)) {}

    void process(bool ok) override {

    }

};

}