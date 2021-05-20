/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <utility>

#include "cid.hpp"
#include "codec/cbor/cbor_token.hpp"
#include "storage/ipld/light_ipld.hpp"

namespace fc::codec::cbor::light_reader {
  using storage::ipld::LightIpldPtr;

  class HamtWalk {
   public:
    inline HamtWalk(LightIpldPtr ipld, Hash256 root) : ipld{std::move(ipld)} {
      cids.push_back(root);
    }

    inline bool empty() const {
      return node.empty() && next_cid == cids.size();
    }

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
          if (ipld->get(cids[next_cid++], _node)) {
            node = _node;
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
          } else {
            _node.resize(0);
            node = {};
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
            cids.push_back(*cid);
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

   private:
    LightIpldPtr ipld;
    std::vector<Hash256> cids;
    size_t next_cid{};
    Buffer _node;
    BytesIn node;
    size_t _bucket{};
  };
}  // namespace fc::codec::cbor::light_reader
