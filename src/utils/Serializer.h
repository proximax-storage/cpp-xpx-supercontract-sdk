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

namespace sirius {

template<class T>
std::string serialize( const T& value ) {
    std::ostringstream os( std::ios::binary );
    cereal::PortableBinaryOutputArchive archive( os );
    archive( value );
    return os.str();
}

template<class T>
T deserialize( const std::string& value ) {
    std::istringstream is( value, std::ios::binary );
    cereal::PortableBinaryInputArchive iarchive(is);
    T info;
    iarchive( info );
    return info;
}

}