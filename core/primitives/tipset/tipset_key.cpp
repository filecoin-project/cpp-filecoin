/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/tipset/tipset_key.hpp"

#include <libp2p/multi/content_identifier_codec.hpp>
#include "codec/cbor/cbor.hpp"
#include "common/outcome.hpp"

namespace fc::primitives::tipset {

  outcome::result<TipsetKey> TipsetKey::createFromCids(
      gsl::span<const CID> cids) {
    OUTCOME_TRY(encoded, detail::encodeKey(cids));
    return TipsetKey{common::Buffer(std::move(encoded))};
  }

  std::string TipsetKey::toPrettyString() const {
    auto cids = getCids();
    if (cids.empty()) {
      return "{}";
    }

    std::string result = "{";
    auto size = cids.size();
    for (size_t i = 0; i < cids.size(); ++i) {
      result += cids[0].toPrettyString("");
      if (i != size - 1) {
        result += ", ";
      }
    }
    result += "}";

    return result;
  }

  namespace detail {
    outcome::result<std::vector<uint8_t>> encodeKey(gsl::span<const CID> cids) {
      common::Buffer buffer{};
      for (auto &c : cids) {
        OUTCOME_TRY(v, c.toString());
        buffer.put(v);
      }
      return buffer.toVector();
    }
  }  // namespace detail

}  // namespace fc::primitives::tipset
