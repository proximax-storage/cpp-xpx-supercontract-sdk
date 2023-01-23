/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include <supercontract/ServiceBuilder.h>
#include "MessageSubscriber.h"
#include "Messenger.h"

namespace sirius::contract::messenger {

class MessengerBuilder: public ServiceBuilder<Messenger> {

protected:

    MessageSubscriber* m_messageSubscriber;

public:

    void setMessageSubscriber(MessageSubscriber* messageSubscriber);

};

}