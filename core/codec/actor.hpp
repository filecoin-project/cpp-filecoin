/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <string_view>

#include "codec/address.hpp"
#include "codec/cbor.hpp"
#include "codec/cid.hpp"

namespace fc::codec::actor {
  inline bool readActor(uint64_t &id,
                        std::string_view &code,
                        const Hash256 *&head,
                        BytesIn key,
                        BytesIn value) {
    if (!address::readId(id, key)) {
      return false;
    }
    cbor::CborToken token;
    if (read(token, value).listCount() != 4) {
      return false;
    }
    BytesIn _cid;
    if (!read(token, value).cidSize() || !read(_cid, value, *token.cidSize())) {
      return false;
    }
    BytesIn _code;
    if (!cid::readRawId(_code, _cid) || !_cid.empty()) {
      return false;
    }
    code = common::span::bytestr(_code);
    if (!cbor::readCborBlake(head, value)) {
      return false;
    }
    return true;
  }
}  // namespace fc::codec::actor
