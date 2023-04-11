/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include "blockchain/BlockContext.h"

namespace sirius::contract::blockchain {

BlockContext::BlockContext() : m_responder(&m_context) {}

}