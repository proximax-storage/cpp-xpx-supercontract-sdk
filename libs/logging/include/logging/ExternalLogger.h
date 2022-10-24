/*
*** Copyright 2021 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include <string>

#pragma once

namespace sirius::logging {

class ExternalLogger {

public:

    virtual ~ExternalLogger() = default;

    virtual void trace( const std::string& ) = 0;

    virtual void debug( const std::string& ) = 0;

    virtual void info( const std::string& ) = 0;

    virtual void warn( const std::string& ) = 0;

    virtual void err( const std::string& ) = 0;

    virtual void critical( const std::string& ) = 0;

};

}