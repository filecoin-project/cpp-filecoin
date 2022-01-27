/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "api/types/ledger_key_info.hpp"
#include "common/outcome.hpp"
#include "crypto/secp256k1/secp256k1_provider.hpp"
#include "crypto/signature/signature.hpp"
#include "primitives/address/address.hpp"
#include "storage/map_prefix/prefix.hpp"

namespace fc::api {
  using crypto::secp256k1::Secp256k1ProviderDefault;
  using crypto::signature::Signature;
  using primitives::address::Address;
  using storage::MapPtr;

  class Ledger {
   public:
    Ledger(const MapPtr &store);

    outcome::result<bool> Has(const Address &address) const;

    outcome::result<Signature> Sign(const Address &address,
                                    const Bytes &data) const;

    outcome::result<Address> ImportKey(const LedgerKeyInfo &key_info) const;

    outcome::result<Address> New() const;

   private:
    MapPtr store;
  };

}  // namespace fc::api
