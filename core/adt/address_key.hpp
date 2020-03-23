/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_ADT_ADDRESS_KEY_HPP
#define CPP_FILECOIN_ADT_ADDRESS_KEY_HPP

#include "primitives/address/address.hpp"

namespace fc::adt {
  struct AddressKeyer {
    using Key = primitives::address::Address;

    static std::string encode(const Key &key);

    static outcome::result<Key> decode(const std::string &key);
  };
}  // namespace fc::adt

#endif  // CPP_FILECOIN_ADT_ADDRESS_KEY_HPP
