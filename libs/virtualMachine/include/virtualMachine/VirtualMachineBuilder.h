/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include <virtualMachine/VirtualMachine.h>
#include "storage/Storage.h"
#include <supercontract/ServiceBuilder.h>

#include <map>

namespace sirius::contract::vm {

class VirtualMachineBuilder : public ServiceBuilder<VirtualMachine> {

protected:

    std::map<CallRequest::CallLevel, uint64_t> m_maxExecutableSizes;
    std::weak_ptr<storage::Storage> m_storage;

public:

    void setMaxExecutableSizes(const std::map<CallRequest::CallLevel, uint64_t> maxExecutableSizes);

    void setStorage(std::weak_ptr<storage::Storage>);

};

}