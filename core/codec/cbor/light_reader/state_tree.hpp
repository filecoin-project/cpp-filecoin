/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "cbor_blake/ipld.hpp"
#include "cid.hpp"
#include "codec/cbor/cbor_token.hpp"
#include "common/blob.hpp"


namespace fc::codec::cbor::light_reader {
  inline bool readStateTree(CbCid &hamt,
                            const CbIpldPtr &ipld,
                            const CbCid &root) {
    hamt = root;
    Bytes value;
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
      const CbCid *cid;
      if (!cbor::readCborBlake(cid, input)) {
        return false;
      }
      hamt = *cid;
    }
    return true;
  }
}  // namespace fc::codec::cbor::light_reader
