/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/hamt/hamt.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(fc::storage::hamt, HamtError, e) {
  using fc::storage::hamt::HamtError;
  switch (e) {
    case HamtError::EXPECTED_CID:
      return "Expected CID";
    default:
      return "Unknown error";
  }
}

namespace fc::storage::hamt {
  const CID kDummyCid({}, {}, libp2p::multi::Multihash::create({}, {}).value());
}  // namespace fc::storage::hamt
