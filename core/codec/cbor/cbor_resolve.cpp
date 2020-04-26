/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "codec/cbor/cbor_resolve.hpp"

OUTCOME_CPP_DEFINE_CATEGORY(fc::codec::cbor, CborResolveError, e) {
  using fc::codec::cbor::CborResolveError;
  switch (e) {
    case CborResolveError::INT_KEY_EXPECTED:
      return "Int key expected";
    case CborResolveError::KEY_NOT_FOUND:
      return "Key not found";
    case CborResolveError::CONTAINER_EXPECTED:
      return "Container expected";
    case CborResolveError::INT_KEY_TOO_BIG:
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
      return outcome::failure(CborResolveError::INT_KEY_EXPECTED);
    } catch (std::out_of_range &) {
      return outcome::failure(CborResolveError::KEY_NOT_FOUND);
    }
    if (chars != str.size()) {
      return outcome::failure(CborResolveError::INT_KEY_EXPECTED);
    }
    if (str[0] == '-') {
      return outcome::failure(CborResolveError::INT_KEY_EXPECTED);
    }
    return value;
  }

  outcome::result<std::pair<std::vector<uint8_t>, Path>> resolve(
      gsl::span<const uint8_t> node, Path path) {
    try {
      CborDecodeStream stream(node);
      auto part = path.begin();
      for (; part != path.end(); part++) {
        if (stream.isCid()) {
          break;
        }
        if (stream.isList()) {
          OUTCOME_TRY(index, parseIndex(*part));
          if (index >= stream.listLength()) {
            return CborResolveError::KEY_NOT_FOUND;
          }
          stream = stream.list();
          for (size_t i = 0; i < index; i++) {
            stream.next();
          }
        } else if (stream.isMap()) {
          auto map = stream.map();
          if (map.find(*part) == map.end()) {
            return CborResolveError::KEY_NOT_FOUND;
          }
          stream = map.at(*part);
        } else {
          return CborResolveError::CONTAINER_EXPECTED;
        }
      }
      return std::make_pair(stream.raw(), path.subspan(part - path.begin()));
    } catch (std::system_error &e) {
      return outcome::failure(e.code());
    }
  }
}  // namespace fc::codec::cbor
