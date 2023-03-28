/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include <vector>
#include <string>
#include <memory>
#include "FilesystemTraversal.h"

namespace sirius::contract::storage {

class FilesystemEntry {

public:

    virtual ~FilesystemEntry() = default;

    virtual bool isFile() const = 0;

    virtual std::string name() const = 0;

    virtual void addChild(std::unique_ptr<FilesystemEntry>&& child) = 0;

    virtual void acceptTraversal( FilesystemTraversal& traversal ) const = 0;

};

}