/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "Storage.h"
#include <common/ServiceBuilder.h>

namespace sirius::contract::storage {

class RPCStorageBuilder: public ServiceBuilder<Storage> {

private:

    std::string m_address;

public:

    RPCStorageBuilder(const std::string& address);

    std::shared_ptr<Storage> build(GlobalEnvironment& environment) override;

};

}