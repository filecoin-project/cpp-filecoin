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
#include "vm/actor/actor.hpp"

namespace fc::codec::cbor::light_reader {
  using common::Hash256;
  using vm::actor::ActorVersion;

  /**
   * Extracts claims CID from CBORed StoragePower actor state.
   * @param ipld - Light Ipld
   * @param state_root - hash256 from StoragePower actor state root CID
   * @param actor_version - StoragePower actor version
   * returns claims - hash256 from claims HAMT root CID on success, otherwise
   * error
   */
  outcome::result<Hash256> readStoragePowerActorClaims(
      const CbIpldPtr &ipld,
      const Hash256 &state_root,
      ActorVersion actor_version) {
    const static auto kParseError =
        ERROR_TEXT("StoragePowerActor compression: CBOR parsing error");

    Buffer encoded_state;
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
    if (actor_version == ActorVersion::kVersion0) {
      if (!skipNested(input, 1)) {
        return kParseError;
      }
    }
    // claims
    const Hash256 *claims;
    if (!cbor::readCborBlake(claims, input)) {
      return kParseError;
    }
    return *claims;
  }
}  // namespace fc::codec::cbor::light_reader
