/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "GlobalEnvironment.h"

#include <memory>

namespace sirius::contract {

template<class TService>
class ServiceBuilder {

public:

    virtual ~ServiceBuilder() = default;

    virtual std::shared_ptr<TService> build(GlobalEnvironment& environment) = 0;

};

}