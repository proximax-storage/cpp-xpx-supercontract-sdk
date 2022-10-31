/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "StorageModifier.h"
#include "StorageObserver.h"

namespace sirius::contract::storage {

class Storage: public StorageModifier, public StorageObserver {

};

}