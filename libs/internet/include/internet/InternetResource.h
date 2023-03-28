/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include <common/AsyncQuery.h>

namespace sirius::contract::internet {

class InternetResource;

using InternetResourceContainer = std::shared_ptr<InternetResource>;

class InternetResource {

public:

    virtual ~InternetResource() = default;

    virtual void open(std::shared_ptr<AsyncQueryCallback<InternetResourceContainer>>) = 0;

    virtual void read(std::shared_ptr<AsyncQueryCallback<std::vector<uint8_t>>>) = 0;

    virtual void close() = 0;

};

}