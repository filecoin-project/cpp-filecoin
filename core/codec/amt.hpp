/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "codec/cbor.hpp"
#include "codec/cid.hpp"
#include "storage/ipld/ipld2.hpp"

namespace fc::codec::amt {
  struct AmtWalk {
    Ipld2Ptr ipld;
    std::vector<Hash256> cids;
    size_t next_cid{};
    Buffer _node;
    BytesIn node;
    size_t _values{};

    inline AmtWalk(Ipld2Ptr ipld, Hash256 root) : ipld{ipld} {
      cids.push_back(root);
    }

    inline bool _readNode() {
      cbor::CborToken token;
      if (read(token, node).listCount() != 3) {
        return false;
      }
      if (!read(token, node).bytesSize() || !read(node, *token.bytesSize())) {
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
        cids.push_back(*cid);
      }
      if (!read(token, node).listCount()) {
        return false;
      }
      _values = *token.listCount();
      return true;
    }

    inline bool load() {
      cbor::CborToken token;
      if (!ipld->get(cids[next_cid++], _node)) {
        return false;
      }
      node = _node;
      if (!read(token, node).listCount()) {
        return false;
      }
      if (token.listCount() == 4) {
        if (!read(token, node).asUint()) {
          return false;
        }
      } else if (token.listCount() != 3) {
        return false;
      }
      if (!read(token, node).asUint()) {
        return false;
      }
      if (!read(token, node).asUint()) {
        return false;
      }
      if (!_readNode()) {
        return false;
      }
      return true;
    }

    inline bool empty() const {
      return node.empty() && next_cid == cids.size();
    }

    inline bool next(BytesIn &value) {
      while (!empty()) {
        if (_values) {
          --_values;
          if (!codec::cbor::readNested(value, node)) {
            return false;
          }
          return true;
        } else {
          if (!node.empty()) {
            return false;
          }
          if (ipld->get(cids[next_cid++], _node)) {
            node = _node;
            if (!_readNode()) {
              return false;
            }
          } else {
            _node.resize(0);
            node = {};
          }
        }
      }
      return false;
    }
  };

  inline bool msgMeta(const Hash256 *&bls,
                      const Hash256 *&secp,
                      Ipld2Ptr ipld,
                      const Hash256 &cid) {
    Buffer value;
    if (!ipld->get(cid, value)) {
      return false;
    }
    BytesIn input{value};
    cbor::CborToken token;
    return read(token, input).listCount() == 2
           && cbor::readCborBlake(bls, input)
           && cbor::readCborBlake(secp, input);
  }
}  // namespace fc::codec::amt
