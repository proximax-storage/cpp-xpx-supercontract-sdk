/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include <vector>
#include "storage/File.h"

namespace sirius::contract::storage {

File::File(GlobalEnvironment& environment, std::string name)
        : m_environment(environment)
        , m_name(std::move(name)) {}

bool File::isFile() const {
    return true;
}

std::string File::name() const {
    return m_name;
}

void File::addChild(std::unique_ptr<FilesystemEntry>&& child) {
}

void File::acceptTraversal(FilesystemTraversal& traversal) const {
    traversal.acceptFile(*this);
}

}