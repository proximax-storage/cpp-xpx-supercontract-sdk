/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#include <utility>
#include <vector>
#include "storage/Folder.h"

namespace sirius::contract::storage {

Folder::Folder(GlobalEnvironment& environment, std::string name)
        : m_environment(environment), m_name(std::move(name)) {}

bool Folder::isFile() const {
    return false;
}

std::string Folder::name() const {
    return m_name;
}

void Folder::addChild(std::unique_ptr<FilesystemEntry>&& child) {
    auto name = child->name();
    m_children[name] = std::move(child);
}

const std::map <std::string, std::unique_ptr<FilesystemEntry>>& Folder::children() const {
    return m_children;
}

void Folder::acceptTraversal(FilesystemTraversal& traversal) const {
    traversal.acceptFolder(*this);
}

}