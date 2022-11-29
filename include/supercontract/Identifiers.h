/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include <utils/types.h>

namespace sirius::contract {

    DECL_KEY( ContractKey );
    DECL_KEY( DriveKey );
    DECL_KEY( ExecutorKey );
    DECL_KEY( CreatorKey );
    DECL_KEY( CallerKey );

    DECL_HASH( CallId )
    DECL_HASH( SessionId )
    DECL_HASH( StorageHash )
    DECL_HASH( ModificationId )
    DECL_HASH( BlockHash )
    DECL_HASH( TransactionHash )
    DECL_HASH( RequestId )

}