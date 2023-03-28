/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

namespace sirius::contract::storage {

class Folder;
class File;

class FilesystemTraversal {

public:

    virtual ~FilesystemTraversal() = default;

    virtual void acceptFolder( const Folder& folder ) = 0;

    virtual void acceptFile ( const File& file ) = 0;

};

}