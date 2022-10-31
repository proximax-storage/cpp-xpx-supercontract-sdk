/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "RPCTag.h"
#include <supercontract/SingleThread.h>
#include <supercontract/AsyncQuery.h>

namespace sirius::contract::messenger {

class WriteSubscribeTag: public RPCTag, private SingleThread {

private:

    GlobalEnvironment& m_environment;
    std::shared_ptr<AsyncQueryCallback<void>> m_callback;

public:

    WriteSubscribeTag(GlobalEnvironment& environment,
                          std::shared_ptr<AsyncQueryCallback<void>>&& callback);

    void process(bool ok) override;

};

}