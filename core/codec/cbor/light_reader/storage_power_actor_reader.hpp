/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "codec/cbor/cbor_token.hpp"
#include "codec/cbor/light_reader/cid.hpp"
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
   * @param[out] claims - read hash256 from claims HAMT root CID
   * returns true on successful parsing, otherwise false
   */
  bool readStoragePowerActorClaims(const LightIpldPtr &ipld,
                                   const Hash256 &state_root,
                                   ActorVersion actor_version,
                                   Hash256 &claims) {
    Buffer encoded_state;
    if (!ipld->get(state_root, encoded_state)) {
      return false;
    }
    BytesIn input{encoded_state};
    cbor::CborToken token;
    if (!read(token, input).listCount()) {
      return false;
    }
    BytesIn nested;
    // total_raw_power
    if (!readNested(nested, input)) {
      return false;
    }
    // total_raw_commited
    if (!readNested(nested, input)) {
      return false;
    }  // total_qa_power
    if (!readNested(nested, input)) {
      return false;
    }  // total_qa_commited
    if (!readNested(nested, input)) {
      return false;
    }  // total_pledge
    if (!readNested(nested, input)) {
      return false;
    }  // this_epoch_raw_power
    if (!readNested(nested, input)) {
      return false;
    }  // this_epoch_qa_power
    if (!readNested(nested, input)) {
      return false;
    }  // this_epoch_pledge
    if (!readNested(nested, input)) {
      return false;
    }  // this_epoch_qa_power_smoothed
    if (!readNested(nested, input)) {
      return false;
    }  // miner_count
    if (!readNested(nested, input)) {
      return false;
    }  // num_miners_meeting_min_power
    if (!readNested(nested, input)) {
      return false;
    }  // cron_event_queue
    if (!readNested(nested, input)) {
      return false;
    }  // first_cron_epoch
    if (!readNested(nested, input)) {
      return false;
    }  // last_processed_cron_epoch for V0
    if (actor_version == ActorVersion::kVersion0) {
      if (!readNested(nested, input)) {
        return false;
      }
    }
    // claims
    const Hash256 *cid;
    if (!cbor::readCborBlake(cid, input)) {
      return false;
    }
    claims = *cid;
    return true;
  }
}  // namespace fc::codec::cbor::light_reader
