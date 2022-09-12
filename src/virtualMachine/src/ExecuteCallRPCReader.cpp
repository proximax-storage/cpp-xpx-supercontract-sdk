/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "ExecuteCallRPCReader.h"

namespace sirius::contract::vm {

ExecuteCallRPCReader::ExecuteCallRPCReader(
        GlobalEnvironment& environment,
        std::shared_ptr<AsyncQueryCallback<std::optional<supercontractserver::Response>>> callback )
        : m_environment( environment )
        , m_callback( std::move(callback) ) {}

void ExecuteCallRPCReader::process( bool ok ) {

    ASSERT(!isSingleThread(), m_environment.logger());

    if (ok) {
        m_callback->postReply( std::move(m_response) );
    }
    else {
        m_callback->postReply({});
    }
}

}