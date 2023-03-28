/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

namespace sirius::contract::vm {

class RPCResponseHandler {

public:

    virtual ~RPCResponseHandler() = default;

    virtual void process() = 0;

};

}