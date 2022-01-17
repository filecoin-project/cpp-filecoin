/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/blob.hpp"
#include "crypto/signature/signature.hpp"

namespace fc::api {
  using SignatureType = crypto::signature::Type;
  using common::Blob;

  struct KeyInfo {
    SignatureType type = SignatureType::kUndefined;
    common::Blob<32> private_key;
  };

}  // namespace fc::api
