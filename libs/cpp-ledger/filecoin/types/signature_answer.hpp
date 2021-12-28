/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "cpp-ledger/common/types.hpp"

namespace ledger::filecoin {

  struct SignatureAnswer {
    Bytes r;
    Bytes s;
    uint8_t v{};
    Bytes derSignature;

    inline bool operator==(const SignatureAnswer &other) const {
      return r == other.r && s == other.s && v == other.v
             && derSignature == other.derSignature;
    }

    inline bool operator!=(const SignatureAnswer &other) const {
      return !(*this == other);
    }

    inline Bytes SignatureBytes() const {
      Bytes out;
      out.reserve(r.size() + s.size() + 1);
      out.insert(out.end(), r.begin(), r.end());
      out.insert(out.end(), s.begin(), s.end());
      out.push_back(v);
      return out;
    }
  };

}  // namespace ledger::filecoin
