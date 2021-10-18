/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "codec/cbor/cbor_errors.hpp"
#include "codec/cbor/streams_annotation.hpp"

namespace fc {
  struct CborRaw {
    Bytes b;
  };
  CBOR_ENCODE(CborRaw, raw) {
    if (raw.b.empty()) {
      outcome::raise(codec::cbor::CborDecodeError::kWrongSize);
    }
    return s << s.wrap(raw.b, 1);
  }
  CBOR_DECODE(CborRaw, raw) {
    raw.b = s.raw();
    return s;
  }
  inline auto operator==(const CborRaw &l, const CborRaw &r) {
    return l.b == r.b;
  }
}  // namespace fc
