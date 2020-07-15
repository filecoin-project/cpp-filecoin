/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "codec/cbor/cbor_resolve.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(fc::codec::cbor, CborResolveError, e) {
  using fc::codec::cbor::CborResolveError;
  switch (e) {
    case CborResolveError::kIntKeyExpected:
      return "Int key expected";
    case CborResolveError::kKeyNotFound:
      return "Key not found";
    case CborResolveError::kContainerExpected:
      return "Container expected";
    case CborResolveError::kIntKeyTooBig:
      return "Int key too big";
    default:
      return "Unknown error";
  }
}

namespace fc::codec::cbor {
  outcome::result<uint64_t> parseIndex(const std::string &str) {
    uint64_t value;
    size_t chars;
    try {
      value = std::stoul(str, &chars);
    } catch (std::invalid_argument &) {
      return outcome::failure(CborResolveError::kIntKeyExpected);
    } catch (std::out_of_range &) {
      return outcome::failure(CborResolveError::kKeyNotFound);
    }
    if (chars != str.size()) {
      return outcome::failure(CborResolveError::kIntKeyExpected);
    }
    if (str[0] == '-') {
      return outcome::failure(CborResolveError::kIntKeyExpected);
    }
    return value;
  }

  outcome::result<void> resolve(CborDecodeStream &stream,
                                const std::string &part) {
    try {
      if (stream.isList()) {
        OUTCOME_TRY(index, parseIndex(part));
        if (index >= stream.listLength()) {
          return CborResolveError::kKeyNotFound;
        }
        stream = stream.list();
        for (size_t i = 0; i < index; i++) {
          stream.next();
        }
      } else if (stream.isMap()) {
        auto map = stream.map();
        auto it = map.find(part);
        if (it == map.end()) {
          return CborResolveError::kKeyNotFound;
        }
        stream = std::move(it->second);
      } else {
        return CborResolveError::kContainerExpected;
      }
    } catch (std::system_error &e) {
      return outcome::failure(e.code());
    }
    return outcome::success();
  }
}  // namespace fc::codec::cbor
