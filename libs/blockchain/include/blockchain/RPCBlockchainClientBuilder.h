/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include <common/ServiceBuilder.h>
#include "Blockchain.h"

namespace sirius::contract::blockchain {

class RPCBlockchainClientBuilder: public ServiceBuilder<blockchain::Blockchain> {

private:

	std::string m_address;

public:

	RPCBlockchainClientBuilder(const std::string& address);

    std::shared_ptr<Blockchain> build(GlobalEnvironment& environment) override;

};

}