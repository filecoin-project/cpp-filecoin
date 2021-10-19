/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "cbor_blake/ipld.hpp"
#include "codec/cbor/cbor_token.hpp"
#include "codec/cbor/light_reader/cid.hpp"
#include "common/error_text.hpp"
#include "common/outcome.hpp"

namespace fc::codec::cbor::light_reader {
  /**
   * Extracts claims CID from CBORed StoragePower actor state.
   * @param ipld - Light Ipld
   * @param state_root - hash256 from StoragePower actor state root CID
   * @param v0 - StoragePower actor version 0
   * returns claims - hash256 from claims HAMT root CID on success, otherwise
   * error
   */
  outcome::result<CbCid> readStoragePowerActorClaims(const CbIpldPtr &ipld,
                                                     const CbCid &state_root,
                                                     bool v0) {
    const static auto kParseError =
        ERROR_TEXT("StoragePowerActor compression: CBOR parsing error");

    Bytes encoded_state;
    if (!ipld->get(state_root, encoded_state)) {
      return kParseError;
    }
    BytesIn input{encoded_state};
    cbor::CborToken token;
    if (!read(token, input).listCount()) {
      return kParseError;
    }
    if (!skipNested(input, 13)) {
      return kParseError;
    }
    if (v0) {
      if (!skipNested(input, 1)) {
        return kParseError;
      }
    }
    // claims
    const CbCid *claims;
    if (!cbor::readCborBlake(claims, input)) {
      return kParseError;
    }
    return *claims;
  }
}  // namespace fc::codec::cbor::light_reader
