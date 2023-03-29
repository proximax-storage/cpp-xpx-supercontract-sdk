/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include <messenger/RPCMessengerBuilder.h>
#include "RPCMessenger.h"

namespace sirius::contract::messenger {

	RPCMessengerBuilder::RPCMessengerBuilder(const std::string& address) : m_address(address) {}

	std::shared_ptr<Messenger> RPCMessengerBuilder::build(GlobalEnvironment& environment) {
		return std::make_shared<RPCMessenger>(environment, m_address, *m_messageSubscriber);
	}

}