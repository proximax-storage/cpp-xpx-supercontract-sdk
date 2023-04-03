/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "RPCTag.h"
#include <future>

namespace sirius::contract::rpcExecutorServer {

class WriteRPCTag: public RPCTag {

private:

    std::promise<bool> m_promise;

public:

    WriteRPCTag(std::promise<bool>&& promise);

    void process(bool ok) override;
};

}