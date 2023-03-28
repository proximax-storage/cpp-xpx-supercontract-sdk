/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include <common/Identifiers.h>

namespace sirius::contract::messenger {

struct OutputMessage {
    ExecutorKey m_receiver;
    std::string m_tag;
    std::string m_content;
};

struct InputMessage {
    std::string m_tag;
    std::string m_content;
};

}