/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "codec/cbor/light_reader/_walk.hpp"
#include "codec/cbor/light_reader/cid.hpp"

namespace fc::codec::cbor::light_reader {
  struct AmtWalk : _Walk {
    using _Walk::_Walk;

    inline bool load() {
      cbor::CborToken token;
      if (!_next()) {
        return false;
      }
      if (!read(token, node).listCount()) {
        return false;
      }
      // skip bit width if present
      if (token.listCount() == 4) {
        if (!read(token, node).asUint()) {
          return false;
        }
      } else if (token.listCount() != 3) {
        return false;
      }
      // skip height
      if (!read(token, node).asUint()) {
        return false;
      }
      // skip count
      if (!read(token, node).asUint()) {
        return false;
      }
      if (!readNode()) {
        return false;
      }
      return true;
    }

    inline bool next(BytesIn &value) {
      while (!empty()) {
        if (_values) {
          --_values;
          return codec::cbor::readNested(value, node);
        }
        if (!node.empty()) {
          return false;
        }
        if (_next() && !readNode()) {
          return false;
        }
      }
      return false;
    }

    inline bool readNode() {
      cbor::CborToken token;
      if (read(token, node).listCount() != 3) {
        return false;
      }
      if (!read(token, node).bytesSize()
          || !codec::read(node, *token.bytesSize())) {
        return false;
      }
      if (!read(token, node).listCount()) {
        return false;
      }
      for (auto links{*token.listCount()}; links; --links) {
        const Hash256 *cid;
        if (!cbor::readCborBlake(cid, node)) {
          return false;
        }
        _push(*cid);
      }
      if (!read(token, node).listCount()) {
        return false;
      }
      _values = *token.listCount();
      return true;
    }

    size_t _values{};
  };
}  // namespace fc::codec::cbor::light_reader
