/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "common/libp2p/cbor_buffering.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(fc::common::libp2p, CborBuffering::Error, e) {
  return "CborBuffering::Error";
}

namespace fc::common::libp2p {
  using Head = CborBuffering::Head;

  outcome::result<Head> Head::first(size_t &more, uint8_t first) {
    // https://tools.ietf.org/html/rfc7049#section-2
    // decode major type
    auto type = Type{(first & 0xe0) >> 5};
    switch (type) {
      case Type::Unsigned:
      case Type::Signed:
      case Type::Bytes:
      case Type::Text:
      case Type::Array:
      case Type::Map:
      case Type::Tag:
      case Type::Special:
        break;
      default:
        return Error::kInvalidHeadType;
    }
    Head head{type, 0};
    // decode additional information length
    first &= 0x1f;
    if (first < 24) {
      // additional information is in first byte
      more = 0;
      head.value = first;
    } else {
      switch (first) {
        case 24:
          more = 1;
          break;
        case 25:
          more = 2;
          break;
        case 26:
          more = 4;
          break;
        case 27:
          more = 8;
          break;
        default:
          return Error::kInvalidHeadValue;
      }
    }
    return head;
  }

  void Head::next(size_t &more, uint8_t next, Head &head) {
    assert(more > 0);
    --more;
    head.value = (head.value << 8) | next;
  }

  bool CborBuffering::done() const {
    return moreBytes() == 0 && more_nested.empty();
  }

  void CborBuffering::reset() {
    assert(done());
    more_nested.push_back(1);
  }

  size_t CborBuffering::moreBytes() const {
    return more_bytes != 0 ? more_bytes : more_nested.empty() ? 0 : 1;
  }

  outcome::result<size_t> CborBuffering::consume(
      gsl::span<const uint8_t> input) {
    assert(!done());
    auto size = static_cast<size_t>(input.size());
    size_t consumed = 0;
    if (!partial_head && more_bytes != 0) {
      auto partial = std::min(more_bytes, size - consumed);
      more_bytes -= partial;
      consumed += partial;
    } else {
      assert(!more_nested.empty());
    }
    while (consumed < size && !more_nested.empty()) {
      if (!partial_head) {
        OUTCOME_TRY(head, Head::first(more_bytes, input[consumed]));
        ++consumed;
        partial_head = head;
      }
      while (consumed < size && more_bytes != 0) {
        Head::next(more_bytes, input[consumed], *partial_head);
        ++consumed;
      }
      if (more_bytes != 0) {
        break;
      }
      auto head = *partial_head;
      partial_head = {};
      --more_nested.back();
      switch (head.type) {
        case Type::Unsigned:
        case Type::Signed:
        case Type::Special:
          break;
        case Type::Bytes:
        case Type::Text: {
          auto partial =
              std::min(static_cast<size_t>(head.value), size - consumed);
          consumed += partial;
          more_bytes = head.value - partial;
          break;
        }
        case Type::Array:
          more_nested.push_back(head.value);
          break;
        case Type::Map:
          more_nested.push_back(2 * head.value);
          break;
        case Type::Tag:
          more_nested.push_back(1);
          break;
      }
      while (!more_nested.empty() && more_nested.back() == 0) {
        more_nested.pop_back();
      }
    }
    return consumed;
  }
}  // namespace fc::common::libp2p
