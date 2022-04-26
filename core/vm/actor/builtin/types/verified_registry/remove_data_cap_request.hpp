/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "crypto/signature/signature.hpp"
#include "primitives/address/address.hpp"

namespace fc::vm::actor::builtin::types::verified_registry {
  using crypto::signature::Signature;
  using primitives::address::Address;

  struct RemoveDataCapRequest {
    Address verifier;
    Signature signature;

    inline bool operator==(const RemoveDataCapRequest &other) const {
      return verifier == other.verifier && signature == other.signature;
    }

    inline bool operator!=(const RemoveDataCapRequest &other) const {
      return !(*this == other);
    }
  };
  CBOR_TUPLE(RemoveDataCapRequest, verifier, signature)

}  // namespace fc::vm::actor::builtin::types::verified_registry
