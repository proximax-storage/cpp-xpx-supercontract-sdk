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
class AcceptRequestTag : public ServerRPCTag {

public:

    std::shared_ptr<TContext> m_context;

private:

    std::shared_ptr<AsyncQueryCallback<std::shared_ptr<TContext>>> m_callback;

public:

	AcceptRequestTag(std::shared_ptr<AsyncQueryCallback<std::shared_ptr<TContext>>>&& callback)
            : m_context(std::make_shared<TContext>())
              , m_callback(std::move(callback)) {}

    void process(bool ok) override {
        if (ok) {
            m_callback->postReply(std::move(m_context));
        }
    }

};

}