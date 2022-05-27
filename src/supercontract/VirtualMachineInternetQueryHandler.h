/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

namespace sirius::contract {

    class VirtualMachineInternetQueryHandler {

    public:

        virtual void openConnection( const std::string& source, std::function<void( uint64_t )> successCallback, std::function<void()> failureCallback ) {
        };

    };

}