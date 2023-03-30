/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include <tl/expected.hpp>
#include <system_error>
#include "executor.pb.h"

namespace sirius::contract::executor {

class RPCTagListener {

public:

    virtual ~RPCTagListener() = default;

    virtual void onStarted(tl::expected<void, std::error_code>&& res) = 0;

    virtual void onRead(tl::expected<executor_server::ServerMessage, std::error_code>&& res) = 0;

    virtual void onWritten(tl::expected<void, std::error_code>&& res) = 0;

    virtual void onFinished(tl::expected<void, std::error_code>&& res) = 0;
};

}