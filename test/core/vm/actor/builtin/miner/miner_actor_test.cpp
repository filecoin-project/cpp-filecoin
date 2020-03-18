/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/miner/miner_actor.hpp"

#include <gtest/gtest.h>
#include "testutil/cbor.hpp"

using fc::primitives::address::Address;
using fc::vm::actor::builtin::miner::MinerActorState;
using fc::vm::actor::builtin::miner::MinerInfo;
using fc::vm::actor::builtin::miner::SectorOnChainInfo;
using fc::vm::actor::builtin::miner::SectorPreCommitInfo;
using fc::vm::actor::builtin::miner::SectorPreCommitOnChainInfo;
using fc::vm::actor::builtin::miner::WorkerKeyChange;
using fc::vm::actor::builtin::miner::RegisteredProof;

const MinerInfo miner_info{
    Address::makeFromId(2),
    Address::makeFromId(3),
    boost::none,
    "\xDE\xAD",
    4,
};

/// Miner actor state CBOR encoding and decoding
TEST(MinerActorTest, MinerActorStateCbor) {
  MinerActorState state{
      "010001020001"_cid,
      "010001020002"_cid,
      {2, 7},
      "010001020003"_cid,
      miner_info,
      {1, 2},
  };
  expectEncodeAndReencode(
      state,
      "86d82a4700010001020001d82a470001000102000243504a01d82a470001000102000385420002420003f662dead04820102"_unhex);
}

TEST(MinerActorTest, MinerSectorInfo) {
  SectorPreCommitInfo info{
      {},
      1,
      "010001020001"_cid,
      2,
      {3},
      4,
  };
  expectEncodeAndReencode(SectorPreCommitOnChainInfo{info, 1, 2},
                          "838501d82a47000100010200010281030442000102"_unhex);
  expectEncodeAndReencode(
      SectorOnChainInfo{info, 1, 2, 3, 4, 5},
      "868501d82a470001000102000102810304014200024200030405"_unhex);
}

/// Miner info CBOR encoding and decoding
TEST(MinerActorTest, MinerInfoCbor) {
  expectEncodeAndReencode(miner_info, "85420002420003f662dead04"_unhex);
  auto info2 = miner_info;
  info2.pending_worker_key = WorkerKeyChange{
      Address::makeFromId(6),
      5,
  };
  expectEncodeAndReencode(info2, "85420002420003824200060562dead04"_unhex);
}

TEST(MinerActorTest, EncodeRegisteredProof) {
  RegisteredProof proof{RegisteredProof::StackedDRG512MiBPoSt};
  expectEncodeAndReencode(proof, "08"_unhex);
}
