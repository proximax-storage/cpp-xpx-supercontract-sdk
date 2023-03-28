/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "ReadInputMessageTag.h"

namespace sirius::contract::messenger {

void ReadInputMessageTag::process(bool ok) {

    ASSERT(!isSingleThread(), m_environment.logger())

    if (!ok) {
        auto error = tl::unexpected<std::error_code>(std::make_error_code(std::errc::connection_aborted));
        m_callback->postReply(error);
    } else {
        ASSERT(m_response.has_input_message(), m_environment.logger())
        const auto& message = m_response.input_message();
        InputMessage inputMessage;
        inputMessage.m_tag = message.tag();
        inputMessage.m_content = message.content();
        m_callback->postReply(std::move(inputMessage));
    }
}

ReadInputMessageTag::ReadInputMessageTag(GlobalEnvironment& environment,
                                         std::shared_ptr<AsyncQueryCallback<InputMessage>>&& callback)
        : m_environment(environment)
        , m_callback(std::move(callback)) {}

}