/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "WriteOutputMessageTag.h"

namespace sirius::contract::messenger {

WriteOutputMessageTag::WriteOutputMessageTag(GlobalEnvironment& environment,
                                             std::shared_ptr<AsyncQueryCallback<void>>&& callback)
        : m_environment(environment)
        , m_callback(std::move(callback)) {}

void WriteOutputMessageTag::process(bool ok) {

    ASSERT(!isSingleThread(), m_environment.logger())

    if (!ok) {
        auto error = tl::unexpected<std::error_code>(std::make_error_code(std::errc::connection_aborted));
        m_callback->postReply(error);
    } else {
        m_callback->postReply(expected<void>());
    }
}

}