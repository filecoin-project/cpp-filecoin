/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "codec/cbor.hpp"
#include "codec/cid.hpp"
#include "storage/ipld/ipld2.hpp"

namespace fc::codec::hamt {
  struct HamtWalk {
    Ipld2Ptr ipld;
    std::vector<Hash256> cids;
    size_t next_cid{};
    Buffer _node;
    BytesIn node;
    size_t _bucket{};

    inline HamtWalk(Ipld2Ptr ipld, Hash256 root) : ipld{ipld} {
      cids.push_back(root);
    }

    inline bool empty() const {
      return node.empty() && next_cid == cids.size();
    }

    inline bool next(BytesIn &key, BytesIn &value) {
      cbor::CborToken token;
      while (!empty()) {
        if (_bucket) {
          --_bucket;
          if (read(token, node).listCount() != 2) {
            return false;
          }
          if (!read(token, node).bytesSize()) {
            return false;
          }
          if (!read(key, node, *token.bytesSize())) {
            return false;
          }
          if (!codec::cbor::readNested(value, node)) {
            return false;
          }
          return true;
        } else if (node.empty()) {
          if (ipld->get(cids[next_cid++], _node)) {
            node = _node;
            if (read(token, node).listCount() != 2) {
              return false;
            }
            if (!read(token, node).bytesSize()
                || !read(node, *token.bytesSize())) {
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
            if (!read(str, node, 1) || (str[0] != '0' && str[0] != '1')) {
              return false;
            }
          }
          if (!read(token, node)) {
            return false;
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
  };

  inline bool stateTree(Hash256 &hamt, Ipld2Ptr ipld, const Hash256 &root) {
    hamt = root;
    Buffer value;
    if (!ipld->get(root, value)) {
      return false;
    }
    BytesIn input{value};
    cbor::CborToken token;
    if (!read(token, input).listCount()) {
      return false;
    }
    if (token.listCount() == 3) {
      if (!read(token, input).asUint()) {
        return false;
      }
      const Hash256 *cid;
      if (!cbor::readCborBlake(cid, input)) {
        return false;
      }
      hamt = *cid;
    }
    return true;
  }
}  // namespace fc::codec::hamt
