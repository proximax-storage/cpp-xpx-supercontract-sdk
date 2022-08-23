/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "supercontract/AsyncQuery.h"

namespace sirius::contract::internet {

class InternetResource;
using InternetResourceContainer = std::shared_ptr<InternetResource>;

class InternetResource {

public:

    virtual ~InternetResource() = default;

    virtual void open( const std::shared_ptr<AsyncCallback<InternetResourceContainer>>& ) = 0;

    virtual void read( const std::shared_ptr<AsyncCallback<std::optional<std::vector<uint8_t>>>>& ) = 0;

    virtual void close() = 0;

};

}