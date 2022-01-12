/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <string>
#include "api/types/key_info.hpp"
#include "api/types/tipset_context.hpp"
#include "common/bytes.hpp"
#include "common/outcome.hpp"
#include "crypto/signature/signature.hpp"
#include "primitives/address/address.hpp"
#include "primitives/types.hpp"

namespace fc::api {
  using crypto::signature::Signature;
  using primitives::TokenAmount;
  using primitives::address::Address;

  class WalletApi {
   public:
    virtual ~WalletApi() = default;

    virtual outcome::result<TokenAmount> WalletBalance(
        const TipsetContext &context, const Address &address) const = 0;

    virtual outcome::result<Address> WalletDefaultAddress() const = 0;

    virtual outcome::result<bool> WalletHas(const TipsetContext &context,
                                            const Address &address) const = 0;

    virtual outcome::result<Address> WalletImport(
        const KeyInfo &info) const = 0;

    virtual outcome::result<Address> WalletNew(
        const std::string &type) const = 0;

    virtual outcome::result<void> WalletSetDefault(
        const Address &address) const = 0;

    virtual outcome::result<Signature> WalletSign(const TipsetContext &context,
                                                  const Address &address,
                                                  const Bytes &data) const = 0;

    virtual outcome::result<bool> WalletVerify(
        const TipsetContext &context,
        const Address &address,
        const Bytes &data,
        const Signature &signature) const = 0;
  };

}  // namespace fc::api
