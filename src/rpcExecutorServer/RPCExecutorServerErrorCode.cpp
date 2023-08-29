/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "RPCExecutorServerErrorCode.h"

namespace sirius::contract::rpcExecutorServer {

const char* RPCExecutorErrorCategory::name() const noexcept {
    return "executor";
}

std::string RPCExecutorErrorCategory::message(int ev) const {
    switch (ev) {
        case static_cast<unsigned int>(RPCExecutorError::read_error):
            return "Executor RPC server read error";
        default:
            return "Other error";
    }
}

bool RPCExecutorErrorCategory::equivalent(int code, const std::error_condition& condition) const noexcept {
    return false;
}

const std::error_category& rpcExecutorErrorCategory() {
    static RPCExecutorErrorCategory instance;
    return instance;
}

std::error_code make_error_code(RPCExecutorError e) {
    return {static_cast<int>(e), RPCExecutorErrorCategory()};
}

}
