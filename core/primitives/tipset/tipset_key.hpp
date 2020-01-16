/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_PRIMITIVES_TIPSET_TIPSET_KEY_HPP
#define CPP_FILECOIN_CORE_PRIMITIVES_TIPSET_TIPSET_KEY_HPP

namespace fc::primitives::tipset {
  /**
   * @struct TipsetKey
   */
  struct TipsetKey {
    std::string value;
  };

  TipsetKey decodeKey(gsl::span<const uint8_t> bytes);

  std::vector<uint8_t> encodeKey(gsl::span<const CID> cids);

  TipsetKey createTipsetKey(gsl::span<CID> cids) {
    auto && encoded =
  }

}  // namespace fc::primitives::tipset

#endif  // CPP_FILECOIN_CORE_PRIMITIVES_TIPSET_TIPSET_KEY_HPP
