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
    for (size_t i = 0; i < cids.size(); ++i) {
      result += cids[0].toPrettyString("");
      if (i != size - 1) {
        result += ", ";
      }
    }
    result += "}";

    return result;
  }

  outcome::result<std::vector<uint8_t>> TipsetKey::toVector() const {
    std::vector<uint8_t> buffer{};
    for (auto &c : cids) {
      OUTCOME_TRY(v, c.toString());
      buffer.insert(buffer.end(), v.begin(), v.end());
    }

    return buffer;
  }

}  // namespace fc::primitives::tipset
