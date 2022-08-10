/**
*** Copyright (c) 2016-present,
*** Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp. All rights reserved.
***
*** This file is part of Catapult.
***
*** Catapult is free software: you can redistribute it and/or modify
*** it under the terms of the GNU Lesser General Public License as published by
*** the Free Software Foundation, either version 3 of the License, or
*** (at your option) any later version.
***
*** Catapult is distributed in the hope that it will be useful,
*** but WITHOUT ANY WARRANTY; without even the implied warranty of
*** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*** GNU Lesser General Public License for more details.
***
*** You should have received a copy of the GNU Lesser General Public License
*** along with Catapult. If not, see <http://www.gnu.org/licenses/>.
**/

#include "crypto/KeyUtils.h"
#include "crypto/PrivateKey.h"
#include "utils/HexParser.h"

namespace sirius { namespace crypto {

	utils::ContainerHexFormatter<Key::const_iterator> FormatKey(const Key& key) {
		return utils::HexFormat(key.cbegin(), key.cend());
	}

	utils::ContainerHexFormatter<Key::const_iterator> FormatKey(const PrivateKey& key) {
		return utils::HexFormat(key.begin(), key.end());
	}

	utils::ContainerHexFormatter<const uint8_t*> FormatKey(const std::string& key) {
		return utils::HexFormat((const uint8_t*)key.data(), (const uint8_t*)key.data() + key.size());
	}

	Key ParseKey(const std::string& keyString) {
		return utils::ParseByteArray<Key>(keyString);
	}

	bool IsValidKeyString(const std::string& str) {
		Key key;
		return utils::TryParseHexStringIntoContainer(str.data(), str.size(), key);
	}
}}
