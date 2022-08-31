/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include <string>
#include <optional>

#include "supercontract/AsyncQuery.h"

namespace sirius::contract::vm {

class VirtualMachineInternetQueryHandler {

public:

    virtual void openConnection(
            const std::string& url,
            std::shared_ptr<AsyncQueryCallback < std::optional<uint64_t>>> callback ) = 0;

    virtual void read(
            uint64_t connectionId,
            std::shared_ptr<AsyncQueryCallback < std::optional<std::vector<uint8_t>>>> callback ) = 0;

    virtual void closeConnection(
            uint64_t connectionId,
            std::shared_ptr<AsyncQueryCallback < bool>> callback ) = 0;

    virtual ~VirtualMachineInternetQueryHandler() = default;
};

}