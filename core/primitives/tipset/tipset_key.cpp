/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/tipset/tipset_key.hpp"

#include <libp2p/multi/content_identifier_codec.hpp>
#include "common/outcome.hpp"

namespace fc::primitives::tipset {

  std::string TipsetKey::toPrettyString() const {
    if (cids.empty()) {
      return "{}";
    }

    std::string result = "{";
    auto size = cids.size();
    for (size_t i = 0; i < size; ++i) {
      // TODO (yuraz): FIL-133 use CID::toString()
      //  after it is implemented for CID V1
      result += cids[i].toPrettyString("");
      if (i != size - 1) {
        result += ", ";
      }
    }
    result += "}";

    return result;
  }

  outcome::result<std::vector<uint8_t>> TipsetKey::toBytes() const {
    std::vector<uint8_t> buffer{};
    for (auto &c : cids) {
      OUTCOME_TRY(v, c.toBytes());
      buffer.insert(buffer.end(), v.begin(), v.end());
    }

    return buffer;
  }

}  // namespace fc::primitives::tipset
