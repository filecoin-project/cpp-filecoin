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
   * Extracts miner_info, sectors, and deadlines CID from CBORed
   * Miner actor state.
   * @param ipld - lightweight ipld
   * @param state_root - Miner actor state root
   * @param v0 - Miner actor version 0
   * @return tuple of CIDs (miner_info, sectors, deadlines) on success,
   * otherwise false
   */
  outcome::result<std::tuple<CbCid, CbCid, CbCid>> readMinerActorInfo(
      const CbIpldPtr &ipld, const CbCid &state_root, bool v0) {
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
    const CbCid *miner_info;
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
    if (!v0) {
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
    const CbCid *sectors;
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
    const CbCid *deadlines;
    if (!cbor::readCborBlake(deadlines, input)) {
      return kParseError;
    }
    return std::make_tuple(*miner_info, *sectors, *deadlines);
  }
}  // namespace fc::codec::cbor::light_reader
