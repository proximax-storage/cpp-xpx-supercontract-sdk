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

#pragma once
#include "crypto/KeyPair.h"
#include "utils/RawBuffer.h"
#include <vector>

namespace sirius { namespace crypto {

	/// Signs data pointed by \a dataBuffer using \a keyPair, placing resulting signature in \a computedSignature.
	/// \note The function will throw if the generated S part of the signature is not less than the group order.
	PLUGIN_API void Sign(const KeyPair& keyPair, const utils::RawBuffer& dataBuffer, Signature& computedSignature);

	/// Signs data in \a buffersList using \a keyPair, placing resulting signature in \a computedSignature.
	/// \note The function will throw if the generated S part of the signature is not less than the group order.
	PLUGIN_API void Sign(const KeyPair& keyPair, std::initializer_list<const utils::RawBuffer> buffersList, Signature& computedSignature);

	/// Verifies that \a signature of data pointed by \a dataBuffer is valid, using public key \a publicKey.
	/// Returns \c true if signature is valid.
	PLUGIN_API bool Verify(const Key& publicKey, const utils::RawBuffer& dataBuffer, const Signature& signature);

	/// Verifies that \a signature of data in \a buffersList is valid, using public key \a publicKey.
	/// Returns \c true if signature is valid.
	PLUGIN_API bool Verify(const Key& publicKey, std::initializer_list<const utils::RawBuffer> buffersList, const Signature& signature);
}}
