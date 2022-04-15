/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "contract/Requests.h"

#include <memory>

namespace sirius::contract {

class VirtualMachine {

public:

    virtual ~VirtualMachine() = default;

    virtual void execute( const CallRequest& ) = 0;

};

}