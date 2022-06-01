/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include <thread>

namespace sirius::contract {

class DebugInfo {

public:

    std::string m_peerName;
    std::thread::id m_threadId;

    DebugInfo( const std::string& peerName, const std::thread::id& threadId )
            : m_peerName( peerName )
            , m_threadId( threadId )
            {}

    DebugInfo( const DebugInfo& debugInfo ) = default;
    DebugInfo( DebugInfo&& debugInfo ) = delete;

};

}