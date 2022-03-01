/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <string>
#include "api/types/key_info.hpp"
#include "api/utils.hpp"
#include "common/bytes.hpp"
#include "crypto/signature/signature.hpp"
#include "primitives/address/address.hpp"
#include "primitives/types.hpp"

namespace fc::api {
  using crypto::signature::Signature;
  using primitives::TokenAmount;
  using primitives::address::Address;
  namespace jwt = primitives::jwt;

  class WalletApi {
   public:
    virtual ~WalletApi() = default;

    API_METHOD(WalletBalance,
               jwt::kReadPermission,
               TokenAmount,
               const Address &)

    API_METHOD(WalletDefaultAddress, jwt::kWritePermission, Address)

    API_METHOD(WalletHas, jwt::kWritePermission, bool, const Address &)

    API_METHOD(WalletImport, jwt::kAdminPermission, Address, const KeyInfo &)

    API_METHOD(WalletNew, jwt::kWritePermission, Address, const std::string &)

    API_METHOD(WalletSetDefault, jwt::kWritePermission, void, const Address &)

    API_METHOD(WalletList, jwt::kAdminPermission, std::vector<Address>)

    API_METHOD(WalletDelete, jwt::kAdminPermission, void, const Address &)

    API_METHOD(WalletSign,
               jwt::kSignPermission,
               Signature,
               const Address &,
               const Bytes &)

    /** Verify signature by address (may be id or key address) */
    API_METHOD(WalletVerify,
               jwt::kReadPermission,
               bool,
               const Address &,
               const Bytes &,
               const Signature &)
  };

  template <typename A, typename F>
  void visitWallet(A &&a, const F &f) {
    f(a.WalletBalance);
    f(a.WalletDefaultAddress);
    f(a.WalletHas);
    f(a.WalletImport);
    f(a.WalletList);
    f(a.WalletNew);
    f(a.WalletSetDefault);
    f(a.WalletSign);
    f(a.WalletVerify);
  }

}  // namespace fc::api
