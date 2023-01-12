/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include <string>
#include <optional>

#include "supercontract/AsyncQuery.h"
#include <internet/InternetErrorCode.h>

namespace sirius::contract::vm {

class VirtualMachineInternetQueryHandler {

public:

    virtual void openConnection(
            const std::string& url,
            std::shared_ptr<AsyncQueryCallback<uint64_t>> callback) {
        callback->postReply(tl::unexpected<std::error_code>(internet::make_error_code(internet::InternetError::incorrect_query)));
    }

    virtual void read(
            uint64_t connectionId,
            std::shared_ptr<AsyncQueryCallback<std::vector<uint8_t>>> callback) {
        callback->postReply(tl::unexpected<std::error_code>(internet::make_error_code(internet::InternetError::incorrect_query)));
    }

    virtual void closeConnection(
            uint64_t connectionId,
            std::shared_ptr<AsyncQueryCallback<void>> callback) {
        callback->postReply(tl::unexpected<std::error_code>(internet::make_error_code(internet::InternetError::incorrect_query)));
    }

    virtual ~VirtualMachineInternetQueryHandler() = default;
};

}