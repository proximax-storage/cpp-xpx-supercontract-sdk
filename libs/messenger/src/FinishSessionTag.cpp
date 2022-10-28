/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "FinishSessionTag.h"

namespace sirius::contract::messenger {

FinishSessionTag::FinishSessionTag(GlobalEnvironment& environment)
        : m_environment(environment)
        {}

void FinishSessionTag::process(bool ok) {

    ASSERT(!isSingleThread(), m_environment.logger())

}

}
