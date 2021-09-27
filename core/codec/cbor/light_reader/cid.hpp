/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "cbor_blake/cid.hpp"
#include "codec/cbor/cbor_token.hpp"
#include "codec/uvarint.hpp"

namespace fc::codec::cbor::light_reader {
  constexpr BytesN<3> kRawIdPrefix{0x01, 0x55, 0x00};

  /**
   * Reads CID as Blake2b_256 hash
   * @param[out] key - hash read
   * @param input - bytes
   * @return if input is not Blake2b_256 returns false and key is set to nullptr
   */
  inline bool readCborBlake(const CbCid *&key, BytesIn &input) {
    BytesIn hash;
    if (readPrefix(input, kCborBlakePrefix)
        && codec::read(hash, input, sizeof(CbCid))) {
      // TODO (a.chernyshov) Think about better way to return CID
      // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
      key = reinterpret_cast<const CbCid *>(hash.data());
      return true;
    }
    key = nullptr;
    return false;
  }

  inline bool readRawId(BytesIn &key, BytesIn &input) {
    key = {};
    return readPrefix(input, kRawIdPrefix) && uvarint::readBytes(key, input);
  }
}  // namespace fc::codec::cbor::light_reader

namespace fc::codec::cbor {
  inline bool readCborBlake(const CbCid *&key,
                            const CborToken &token,
                            BytesIn &input) {
    BytesIn cid;
    return token.cidSize() && fc::codec::read(cid, input, *token.cidSize())
           && light_reader::readCborBlake(key, cid) && cid.empty();
  }

  inline bool readCborBlake(const CbCid *&key, BytesIn &input) {
    CborToken token;
    return read(token, input) && readCborBlake(key, token, input);
  }
}  // namespace fc::codec::cbor
