/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "codec/cbor/cbor_token.hpp"
#include "codec/cbor/light_reader/cid.hpp"
#include "common/error_text.hpp"
#include "common/outcome.hpp"
#include "storage/ipld/light_ipld.hpp"
#include "vm/actor/actor.hpp"

namespace fc::codec::cbor::light_reader {
  using common::Hash256;
  using storage::ipld::LightIpldPtr;
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
      const LightIpldPtr &ipld,
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
    BytesIn nested;
    // total_raw_power
    if (!readNested(nested, input)) {
      return kParseError;
    }
    // total_raw_commited
    if (!readNested(nested, input)) {
      return kParseError;
    }  // total_qa_power
    if (!readNested(nested, input)) {
      return kParseError;
    }  // total_qa_commited
    if (!readNested(nested, input)) {
      return kParseError;
    }  // total_pledge
    if (!readNested(nested, input)) {
      return kParseError;
    }  // this_epoch_raw_power
    if (!readNested(nested, input)) {
      return kParseError;
    }  // this_epoch_qa_power
    if (!readNested(nested, input)) {
      return kParseError;
    }  // this_epoch_pledge
    if (!readNested(nested, input)) {
      return kParseError;
    }  // this_epoch_qa_power_smoothed
    if (!readNested(nested, input)) {
      return kParseError;
    }  // miner_count
    if (!readNested(nested, input)) {
      return kParseError;
    }  // num_miners_meeting_min_power
    if (!readNested(nested, input)) {
      return kParseError;
    }  // cron_event_queue
    if (!readNested(nested, input)) {
      return kParseError;
    }  // first_cron_epoch
    if (!readNested(nested, input)) {
      return kParseError;
    }  // last_processed_cron_epoch for V0
    if (actor_version == ActorVersion::kVersion0) {
      if (!readNested(nested, input)) {
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
