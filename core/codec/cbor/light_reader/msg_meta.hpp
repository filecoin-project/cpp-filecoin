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

namespace fc::codec::cbor::light_reader {
  using storage::ipld::LightIpldPtr;

  inline bool readMsgMeta(const Hash256 *&bls,
                          const Hash256 *&secp,
                          LightIpldPtr ipld,
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
}  // namespace fc::codec::cbor
