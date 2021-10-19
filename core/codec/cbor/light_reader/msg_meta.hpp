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
  inline bool readMsgMeta(const CbCid *&bls,
                          const CbCid *&secp,
                          CbIpldPtr ipld,
                          const CbCid &cid) {
    Bytes value;
    if (!ipld->get(cid, value)) {
      return false;
    }
    BytesIn input{value};
    cbor::CborToken token;
    return read(token, input).listCount() == 2
           && cbor::readCborBlake(bls, input)
           && cbor::readCborBlake(secp, input);
  }
}  // namespace fc::codec::cbor::light_reader
