/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "RPCExecutorClientErrorCode.h"

namespace sirius::contract::executor {

const char* RPCExecutorErrorCategory::name() const noexcept {
    return "executor";
}

std::string RPCExecutorErrorCategory::message(int ev) const {
    switch (ev) {
        case static_cast<unsigned int>(RPCExecutorError::read_error):
            return "Executor RPC read error";
        case static_cast<unsigned int>(RPCExecutorError::write_error):
            return "Executor RPC write error";
        case static_cast<unsigned int>(RPCExecutorError::start_error):
            return "Executor RPC start error";
        case static_cast<unsigned int>(RPCExecutorError::finish_error):
            return "Executor RPC finish error";
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
