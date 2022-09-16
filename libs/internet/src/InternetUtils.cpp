/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include <openssl/ocsp.h>

#include "internet/InternetUtils.h"
//#include "log.h"

namespace sirius::contract {

std::optional<URLDescription> parseURL( const std::string& url ) {
    char* host;
    char* port;
    char* target;
    int ssl;

    if ( !OCSP_parse_url( url.c_str(), &host, &port, &target, &ssl )) {
        return {};
    }

    URLDescription urlDescription = {static_cast<bool>(ssl), std::string( host ), std::string( port ),
                                     std::string( target )};

    OPENSSL_free( host );
    OPENSSL_free( target );
    OPENSSL_free( port );

    return urlDescription;
}

}