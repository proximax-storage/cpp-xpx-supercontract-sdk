/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include "../include/utils/Serializer.h"

namespace sirius::utils {

void copyToVector( std::vector <uint8_t>& data, const uint8_t* p, size_t bytes ) {
    data.insert( data.end(), p, p + bytes );
}

}