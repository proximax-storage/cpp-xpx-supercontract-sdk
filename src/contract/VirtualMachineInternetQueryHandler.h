/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

namespace sirius::contract {

    class VirtualMachineInternetQueryHandler {

    public:

        virtual void openConnection(
                const std::string& url,
                std::function<void( std::optional<uint64_t>&& )>&& callback,
                std::function<void()>&& terminateCallback ) = 0;

        virtual void read(
                uint64_t connectionId,
                std::function<void( std::optional<std::vector<uint8_t>>&& )>&& callback,
                std::function<void()>&& terminateCallback ) = 0;

        virtual void closeConnection(
                uint64_t connectionId,
                std::function<void( bool )>&& callback,
                std::function<void()>&& terminateCallback ) = 0;

        virtual void terminate() = 0;

    };

}