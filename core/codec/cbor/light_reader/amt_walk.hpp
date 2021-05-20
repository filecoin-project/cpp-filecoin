/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <utility>

#include "codec/cbor/cbor_token.hpp"
#include "codec/cbor/light_reader/cid.hpp"
#include "storage/ipld/light_ipld.hpp"

namespace fc::codec::cbor::light_reader {
  using storage::ipld::LightIpldPtr;

  class AmtWalk {
   public:
    inline AmtWalk(LightIpldPtr ipld, Hash256 root) : ipld{std::move(ipld)} {
      cids.push_back(root);
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

    inline bool empty() const {
      return node.empty() && next_cid == cids.size();
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
        if (ipld->get(cids[next_cid++], _node)) {
          node = _node;
          if (!readNode()) {
            return false;
          }
        } else {
          _node.resize(0);
          node = {};
        }
      }
      return false;
    }

   private:
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
        cids.push_back(*cid);
      }
      if (!read(token, node).listCount()) {
        return false;
      }
      _values = *token.listCount();
      return true;
    }

    LightIpldPtr ipld;
    /** All CIDs - visited and not visited yet */
    std::vector<Hash256> cids;
    size_t next_cid{};
    Buffer _node;
    BytesIn node;
    size_t _values{};
  };
}  // namespace fc::codec::cbor::light_reader
