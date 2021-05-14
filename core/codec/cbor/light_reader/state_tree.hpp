/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "cid.hpp"
#include "codec/cbor/cbor_token.hpp"
#include "common/blob.hpp"
#include "common/buffer.hpp"
#include "storage/ipld/light_ipld.hpp"

namespace fc::codec {
  inline bool readStateTree(Hash256 &hamt,
                            const Ipld2Ptr &ipld,
                            const Hash256 &root) {
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
}  // namespace fc::codec
