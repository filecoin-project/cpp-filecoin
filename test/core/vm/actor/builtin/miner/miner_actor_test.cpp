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
using fc::vm::actor::builtin::miner::PreCommittedSector;
using fc::vm::actor::builtin::miner::SectorPreCommitInfo;

/// Miner actor state CBOR encoding and decoding
TEST(MinerActorTest, MinerActorStateCbor) {
  MinerActorState state{
    {{0xCAFE, PreCommittedSector{
      SectorPreCommitInfo{
        0xCAFE,
        "402fe713e2328b6f21f07730248b0e6cdce7c7feffe744ec97989e4318d4c5d3"_blob32,
        7,
        {9},
      },
      8,
    }}},
    "010001020002"_cid,
    "010001020003"_cid,
    "010001020004"_cid,
    {2, 7},
    2,
    3,
    false,
    4,
    5,
  };
  expectEncodeAndReencode(state, "8aa163fe9503828419cafe5820402fe713e2328b6f21f07730248b0e6cdce7c7feffe744ec97989e4318d4c5d307810908d82a4700010001020002d82a4700010001020003d82a470001000102000443504a0102420003f40405"_unhex);
}

/// Miner info CBOR encoding and decoding
TEST(MinerActorTest, MinerInfoCbor) {
  MinerInfo info{
    Address::makeFromId(2),
    Address::makeFromId(3),
    {
      Address::makeFromId(6),
      5,
    },
    "\xDE\xAD",
    4,
  };
  expectEncodeAndReencode(info, "85420002420003824200060562dead04"_unhex);
}
