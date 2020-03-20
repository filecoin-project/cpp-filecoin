/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_ADT_ADDRESS_KEY_HPP
#define CPP_FILECOIN_ADT_ADDRESS_KEY_HPP

#include "primitives/address/address.hpp"

namespace fc::adt {
  struct AddressKey {
    using Type = primitives::address::Address;

    static std::string encode(const Type &key);

    static outcome::result<Type> decode(const std::string &key);
  };
}  // namespace fc::adt

#endif  // CPP_FILECOIN_ADT_ADDRESS_KEY_HPP
