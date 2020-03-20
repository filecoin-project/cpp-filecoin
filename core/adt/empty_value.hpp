/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_ADT_EMPTY_VALUE_HPP
#define CPP_FILECOIN_ADT_EMPTY_VALUE_HPP

#include "codec/cbor/streams_annotation.hpp"

namespace fc::adt {
  struct EmptyValue {
    static constexpr uint8_t encoded[1]{0x80};
  };
  CBOR_ENCODE(EmptyValue, empty) {
    return s << s.list();
  }
  CBOR_DECODE(EmptyValue, empty) {
    s.list();
    return s;
  }
}  // namespace fc::adt

#endif  // CPP_FILECOIN_ADT_EMPTY_VALUE_HPP
