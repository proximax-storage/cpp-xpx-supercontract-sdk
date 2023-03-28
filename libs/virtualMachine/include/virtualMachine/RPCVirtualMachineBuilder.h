/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "VirtualMachineBuilder.h"

namespace sirius::contract::vm {

class RPCVirtualMachineBuilder: public VirtualMachineBuilder {

private:

    std::string m_address;

public:

    RPCVirtualMachineBuilder(const std::string& address);

    std::shared_ptr<VirtualMachine> build(GlobalEnvironment& environment) override;

};

}