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
   * Extracts miner_info, sectors, and deadlines CID as Hash256 from CBORed
   * Miner actor state.
   * @param ipld - lightweight ipld
   * @param state_root - Miner actor state root
   * @param actor_version - Miner actor version
   * @return tuple of CIDs (miner_info, sectors, deadlines) on success,
   * otherwise false
   */
  outcome::result<std::tuple<Hash256, Hash256, Hash256>> readMinerActorInfo(
      const CbIpldPtr &ipld,
      const Hash256 &state_root,
      ActorVersion actor_version) {
    const static auto kParseError =
        ERROR_TEXT("MinerActor compression: CBOR parsing error");

    Buffer encoded_state;
    if (!ipld->get(state_root, encoded_state)) {
      return kParseError;
    }
    BytesIn input{encoded_state};
    cbor::CborToken token;
    if (!read(token, input).listCount()) {
      return kParseError;
    }
    // read miner info CID
    const Hash256 *miner_info;
    if (!cbor::readCborBlake(miner_info, input)) {
      return kParseError;
    }
    BytesIn nested;
    // precommit_deposit
    if (!readNested(nested, input)) {
      return kParseError;
    }
    // locked_funds
    if (!readNested(nested, input)) {
      return kParseError;
    }
    // vesting_funds
    if (!readNested(nested, input)) {
      return kParseError;
    }
    // fee_debt for actor version > 0
    if (actor_version != ActorVersion::kVersion0) {
      if (!readNested(nested, input)) {
        return kParseError;
      }
    }
    // initial_pledge_requirement
    if (!readNested(nested, input)) {
      return kParseError;
    }
    // precommitted_sectors
    if (!readNested(nested, input)) {
      return kParseError;
    }
    // precommitted_setctors_expiry
    if (!readNested(nested, input)) {
      return kParseError;
    }
    // allocated_sectors
    if (!readNested(nested, input)) {
      return kParseError;
    }
    // sectors
    const Hash256 *sectors;
    if (!cbor::readCborBlake(sectors, input)) {
      return kParseError;
    }
    // proving_period_start
    if (!readNested(nested, input)) {
      return kParseError;
    }
    // current_deadline
    if (!readNested(nested, input)) {
      return kParseError;
    }
    // deadlines
    const Hash256 *deadlines;
    if (!cbor::readCborBlake(deadlines, input)) {
      return kParseError;
    }
    return std::make_tuple(*miner_info, *sectors, *deadlines);
  }
}  // namespace fc::codec::cbor::light_reader
