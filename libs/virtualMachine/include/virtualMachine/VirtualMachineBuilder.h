/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include <virtualMachine/VirtualMachine.h>
#include "storage/StorageObserver.h"
#include <supercontract/ServiceBuilder.h>

namespace sirius::contract::vm {

class VirtualMachineBuilder : public ServiceBuilder<VirtualMachine> {

protected:

    std::weak_ptr<storage::StorageObserver> m_storageContentObserver;

public:

    void setContentObserver(std::weak_ptr<storage::StorageObserver>);

};

}