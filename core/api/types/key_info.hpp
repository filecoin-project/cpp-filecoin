/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/blob.hpp"
#include "common/bytes.hpp"
#include "common/outcome.hpp"
#include "crypto/signature/signature.hpp"

namespace fc::api {
  using SignatureType = crypto::signature::Type;
  using common::Blob;

  struct KeyInfo {
    SignatureType type = SignatureType::kUndefined;
    Bytes private_key;

    inline outcome::result<Blob<32>> getPrivateKey() const {
      return Blob<32>::fromSpan(private_key);
    }
  };

}  // namespace fc::api
