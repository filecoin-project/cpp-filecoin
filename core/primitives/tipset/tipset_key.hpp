/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_PRIMITIVES_TIPSET_TIPSET_KEY_HPP
#define CPP_FILECOIN_CORE_PRIMITIVES_TIPSET_TIPSET_KEY_HPP

#include "common/buffer.hpp"
#include "primitives/cid/cid.hpp"

namespace fc::primitives::tipset {
  /**
   * @struct TipsetKey implemented according to
   * https://github.com/filecoin-project/lotus/blob/9f3b5bfb6e2c75b77ce41da42aa0a7ba89b5c411/chain/types/tipset_key.go#L27
   */
  struct TipsetKey {
    common::Buffer value;

    static outcome::result<TipsetKey> createFromCids(gsl::span<const CID> cids);

    std::vector<CID> getCids() const;

    std::string toPrettyString() const;
  };

  namespace detail {
    /**
     * @brief decodes CIDs from bytes
     * @param bytes source data
     * @return vector of CIDs
     */
    outcome::result<std::vector<CID>> decodeKey(gsl::span<const uint8_t> bytes);

    /**
     * @brief encodes cids into bytes
     * @return vector of bytes
     */
    outcome::result<std::vector<uint8_t>> encodeKey(gsl::span<const CID> cids);
  }  // namespace detail

}  // namespace fc::primitives::tipset

#endif  // CPP_FILECOIN_CORE_PRIMITIVES_TIPSET_TIPSET_KEY_HPP
