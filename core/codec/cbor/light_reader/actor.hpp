/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <string_view>

#include "cid.hpp"
#include "codec/cbor/cbor_token.hpp"
#include "codec/cbor/light_reader/address.hpp"

namespace fc::codec::cbor::light_reader {
  /**
   * Partially decodes Actor state
   * @param[out] id - actor id address value
   * @param[out] code - actor code id
   * @param[out] head - actor state root CID
   * @param key
   * @param value
   * @return
   */
  inline bool readActor(uint64_t &id,
                        std::string_view &code,
                        const Hash256 *&head,
                        BytesIn key,
                        BytesIn value) {
    if (!readIdAddress(id, key)) {
      return false;
    }
    cbor::CborToken token;
    if (read(token, value).listCount() != 4) {
      return false;
    }
    BytesIn _cid;
    if (!read(token, value).cidSize()
        || !codec::read(_cid, value, *token.cidSize())) {
      return false;
    }
    BytesIn _code;
    if (!readRawId(_code, _cid) || !_cid.empty()) {
      return false;
    }
    code = common::span::bytestr(_code);
    return cbor::readCborBlake(head, value);
  }
}  // namespace fc::codec::cbor::light_reader
