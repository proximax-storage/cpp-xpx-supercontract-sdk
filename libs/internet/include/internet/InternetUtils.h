/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include <string>
#include <optional>

namespace sirius::contract {

    struct URLDescription {
        bool ssl;
        std::string host;
        std::string port;
        std::string target;
    };

    std::optional<URLDescription> parseURL( const std::string& url );
}