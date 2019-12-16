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
  outcome::result<std::pair<std::vector<uint8_t>, Path>> resolve(
      gsl::span<const uint8_t> node, const Path &path) {
    try {
      CborDecodeStream stream(node);
      auto part = path.begin();
      for (; part != path.end(); part++) {
        if (stream.isCid()) {
          break;
        }
        if (stream.isList()) {
          size_t index;
          size_t index_chars;
          try {
            index = std::stoul(*part, &index_chars);
          } catch (std::invalid_argument &) {
            return CborResolveError::INT_KEY_EXPECTED;
          } catch (std::out_of_range &) {
            return CborResolveError::KEY_NOT_FOUND;
          }
          if (index_chars != part->size()) {
            return CborResolveError::INT_KEY_EXPECTED;
          }
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
      return std::make_pair(stream.raw(), Path{part, path.end()});
    } catch (std::system_error &e) {
      return outcome::failure(e.code());
    }
  }
}  // namespace fc::codec::cbor
