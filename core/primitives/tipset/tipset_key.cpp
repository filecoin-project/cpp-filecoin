/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/tipset/tipset_key.hpp"

#include "codec/cbor/cbor.hpp"
#include "common/outcome.hpp"
#include <libp2p/multi/content_identifier_codec.hpp>

namespace fc::primitives::tipset {

  outcome::result<TipsetKey> TipsetKey::createFromBytes(
      gsl::span<const uint8_t> bytes) {
    // check if bytes are encoded key
    OUTCOME_TRY(value, detail::decodeKey(bytes));
    return TipsetKey{common::Buffer{bytes}};
  }

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

    /*
  * func decodeKey(encoded []byte) ([]cid.Cid, error) {
      // To avoid reallocation of the underlying array, estimate the number of
  CIDs to be extracted
      // by dividing the encoded length by the expected CID length.
      estimatedCount := len(encoded) / blockHeaderCIDLen
      cids := make([]cid.Cid, 0, estimatedCount)
      nextIdx := 0
      for nextIdx < len(encoded) {
              nr, c, err := cid.CidFromBytes(encoded[nextIdx:])
              if err != nil {
                      return nil, err
              }
              cids = append(cids, c)
              nextIdx += nr
      }
      return cids, nil
  }
  */

    outcome::result<std::vector<CID>> decodeKey(gsl::span<const uint8_t> bytes) {
      return std::vector<CID>{};
    }
  }  // namespace detail

}  // namespace fc::primitives::tipset
