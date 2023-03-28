/*
*** Copyright 2023 ProximaX Limited. All rights reserved.
*** Use of this source code is governed by the Apache 2.0
*** license that can be found in the LICENSE file.
*/

#pragma once

#include <map>
#include "FilesystemEntry.h"
#include <common/GlobalEnvironment.h>

namespace sirius::contract::storage {

class Folder: public FilesystemEntry {

private:

    GlobalEnvironment& m_environment;
    std::string m_name;
    std::map<std::string, std::unique_ptr<FilesystemEntry>> m_children;

public:

    Folder(GlobalEnvironment& m_environment,
           std::string  name);

    bool isFile() const override;

    std::string name() const override;

    void addChild(std::unique_ptr<FilesystemEntry>&& child) override;

    void acceptTraversal(FilesystemTraversal& traversal) const override;

    const std::map<std::string, std::unique_ptr<FilesystemEntry>>& children() const;
};

}