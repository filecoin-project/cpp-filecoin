/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "codec/cbor/light_reader/cid.hpp"
#include "codec/cbor/light_reader/walk.hpp"

namespace fc::codec::cbor::light_reader {
  struct HamtWalk : Walk {
    using Walk::Walk;

    /**
     * Iterate hamt
     * @param[out] key
     * @param[out] value
     * @return
     */
    inline bool next(BytesIn &key, BytesIn &value) {
      cbor::CborToken token;
      while (!empty()) {
        if (_bucket) {
          --_bucket;
          if (read(token, node).listCount() != 2) {
            return false;
          }
          if (!read(token, node).bytesSize()
              || !codec::read(key, node, *token.bytesSize())) {
            return false;
          }
          if (!codec::cbor::readNested(value, node)) {
            return false;
          }
          return true;
        }
        if (node.empty()) {
          if (_next()) {
            if (read(token, node).listCount() != 2) {
              return false;
            }
            if (!read(token, node).bytesSize()
                || !codec::read(node, *token.bytesSize())) {
              return false;
            }
            if (!read(token, node).listCount()) {
              return false;
            }
          }
        } else {
          if (!read(token, node)) {
            return false;
          }
          if (token.mapCount()) {
            if (token.mapCount() != 1) {
              return false;
            }
            if (read(token, node).strSize() != 1) {
              return false;
            }
            BytesIn str;
            if (!codec::read(str, node, 1)
                || (str[0] != '0' && str[0] != '1')) {
              return false;
            }
            if (!read(token, node)) {
              return false;
            }
          }
          if (token.cidSize()) {
            const Hash256 *cid;
            if (!readCborBlake(cid, token, node)) {
              return false;
            }
            _push(*cid);
          } else {
            if (!token.listCount()) {
              return false;
            }
            _bucket = *token.listCount();
          }
        }
      }
      return false;
    }

    size_t _bucket{};
  };
}  // namespace fc::codec::cbor::light_reader
