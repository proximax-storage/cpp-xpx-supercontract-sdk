/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "contract/AsyncQuery.h"

namespace sirius::contract {

class InternetConnection {

public:

    virtual ~InternetConnection() = default;

    virtual void open( std::weak_ptr<AbstractAsyncQuery<bool>> ) = 0;

    virtual void read( std::weak_ptr<AbstractAsyncQuery<std::optional<std::vector<uint8_t>>>> ) = 0;

};

}