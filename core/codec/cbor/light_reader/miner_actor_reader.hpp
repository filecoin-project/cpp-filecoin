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
   * Extracts miner_info, sectors, and deadlines CID as Hash256 from CBORed
   * Miner actor state.
   * @param ipld - lightweight ipld
   * @param state_root - Miner actor state root
   * @param actor_version - Miner actor version
   * @param[out] miner_info - parsed miner infor CID
   * @param[out] sectors - parsed sectors CID
   * @param[out] deadlines - parsed deadlines CID
   * @return true on successful parsing, otherwise false
   */
  bool readMinerActorInfo(const LightIpldPtr &ipld,
                          const Hash256 &state_root,
                          ActorVersion actor_version,
                          Hash256 &miner_info,
                          Hash256 &sectors,
                          Hash256 &deadlines) {
    Buffer encoded_state;
    if (!ipld->get(state_root, encoded_state)) {
      return false;
    }
    BytesIn input{encoded_state};
    cbor::CborToken token;
    if (!read(token, input).listCount()) {
      return false;
    }
    // read miner info CID
    const Hash256 *cid;
    if (!cbor::readCborBlake(cid, input)) {
      return false;
    }
    miner_info = *cid;
    BytesIn nested;
    // precommit_deposit
    if (!readNested(nested, input)) {
      return false;
    }
    // locked_funds
    if (!readNested(nested, input)) {
      return false;
    }
    // vesting_funds
    if (!readNested(nested, input)) {
      return false;
    }
    // fee_debt for actor version > 0
    if (actor_version != ActorVersion::kVersion0) {
      if (!readNested(nested, input)) {
        return false;
      }
    }
    // initial_pledge_requirement
    if (!readNested(nested, input)) {
      return false;
    }
    // precommitted_sectors
    if (!readNested(nested, input)) {
      return false;
    }
    // precommitted_setctors_expiry
    if (!readNested(nested, input)) {
      return false;
    }
    // allocated_sectors
    if (!readNested(nested, input)) {
      return false;
    }
    // sectors
    if (!cbor::readCborBlake(cid, input)) {
      return false;
    }
    sectors = *cid;
    // proving_period_start
    if (!readNested(nested, input)) {
      return false;
    }
    // current_deadline
    if (!readNested(nested, input)) {
      return false;
    }
    // deadlines
    if (!cbor::readCborBlake(cid, input)) {
      return false;
    }
    deadlines = *cid;
    return true;
  }
}  // namespace fc::codec::cbor::light_reader
