//
// Created by kyrylo on 03.04.2023.
//

#include "StartRPCTag.h"

namespace sirius::contract::rpcExecutorServer {

StartRPCTag::StartRPCTag(std::promise<bool>&& promise) : m_promise(std::move(promise)) {}

void StartRPCTag::process(bool ok) {
    m_promise.set_value(ok);
}

}
