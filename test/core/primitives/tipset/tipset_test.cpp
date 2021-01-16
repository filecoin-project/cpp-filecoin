/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/tipset/tipset.hpp"

#include <gtest/gtest.h>
#include "common/hexutil.hpp"
#include "crypto/blake2/blake2b160.hpp"
#include "primitives/cid/cid_of_cbor.hpp"
#include "testutil/cbor.hpp"
#include "testutil/crypto/sample_signatures.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"

using fc::primitives::cid::getCidOfCbor;

struct TipsetTest : public ::testing::Test {
  using Address = fc::primitives::address::Address;
  using BigInt = fc::primitives::BigInt;
  using BlockHeader = fc::primitives::block::BlockHeader;
  using CID = fc::CID;
  using Signature = fc::primitives::block::Signature;
  using Ticket = fc::primitives::block::Ticket;
  using Tipset = fc::primitives::tipset::Tipset;
  using TipsetError = fc::primitives::tipset::TipsetError;

  BlockHeader makeBlock() {
    auto bls1 =
        "010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101"_blob96;
    auto bls2 =
        "020101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101"_blob96;

    ticket1 = Ticket{fc::Buffer{bls1}};
    ticket2 = Ticket{fc::Buffer{bls2}};

    BlockHeader block_header{
        fc::primitives::address::Address::makeFromId(1),
        ticket2,
        {},
        {fc::primitives::block::BeaconEntry{
            4,
            "F00D"_unhex,
        }},
        {fc::primitives::sector::PoStProof{
            fc::primitives::sector::RegisteredPoStProof::StackedDRG2KiBWinningPoSt,
            "F00D"_unhex,
        }},
        {"010001020002"_cid},
        fc::primitives::BigInt(3),
        4,
        "010001020005"_cid,
        "010001020006"_cid,
        "010001020007"_cid,
        boost::none,
        8,
        boost::none,
        9,
        {},
    };
    return block_header;
  }

  void SetUp() override {
    bh1 = makeBlock();
    bh2 = makeBlock();
    bh2.miner = Address::makeFromId(2);
    bh2.timestamp = 7;
    bh2.ticket = ticket1;
    bh3 = makeBlock();
    bh3.miner = Address::makeFromId(3);
    bh3.height = 3;  // change height
    bh4 = makeBlock();
    bh4.parents.push_back("010001020002"_cid);  // append parent cid

    EXPECT_OUTCOME_TRUE(cid1_, getCidOfCbor(bh1));
    EXPECT_OUTCOME_TRUE(cid2_, getCidOfCbor(bh2));
    EXPECT_OUTCOME_TRUE(cid3_, getCidOfCbor(bh3));
    cid1 = cid1_;
    cid2 = cid2_;
    cid3 = cid3_;

    parent_state_root = "010001020005"_cid;
    parent_weight = BigInt(3);
  }

  BlockHeader bh1, bh2, bh3, bh4;
  CID cid1, cid2, cid3;
  CID parent_state_root;
  Ticket ticket1, ticket2;
  Signature signature;
  BigInt parent_weight;
};

/**
 * @given empty set of blockheaders
 * @when create tipset based on that headers
 * @then creation fails
 */
TEST_F(TipsetTest, CreateEmptyFailure) {
  EXPECT_OUTCOME_FALSE(err, Tipset::create({}));
  ASSERT_EQ(err, TipsetError::kNoBlocks);
}

/**
 * @given invalid set of blockheaders, where parents heights don't match
 * @when create tipset based on that headers
 * @then creation fails
 */
TEST_F(TipsetTest, CreateMismatchingHeightsFailure) {
  EXPECT_OUTCOME_FALSE(err, Tipset::create({bh1, bh3}));
  ASSERT_EQ(err, TipsetError::kMismatchingHeights);
}

/**
 * @given invalid set of blockheaders, where parents are not equal
 * @when create tipset based on that headers
 * @then creation fails
 */
TEST_F(TipsetTest, CreateMismatchingParentsFailure) {
  EXPECT_OUTCOME_FALSE(err, Tipset::create({bh1, bh4}));
  ASSERT_EQ(err, TipsetError::kMismatchingParents);
}

/**
 * @given valid set of blockheaders
 * @when create tipset based on that headers
 * @then creation succeeds and methods return expected values
 */
TEST_F(TipsetTest, CreateSuccess) {
  EXPECT_OUTCOME_TRUE(tipset, Tipset::create({bh1, bh2}));
  const auto &ts = *tipset;

  auto ticket_hash_1 =
      fc::crypto::blake2b::blake2b_256(bh1.ticket.value().bytes);
  auto ticket_hash_2 =
      fc::crypto::blake2b::blake2b_256(bh2.ticket.value().bytes);

  std::vector<CID> cids{cid1, cid2};
  std::vector<CID> cids_reverse{cid2, cid1};
  std::vector<BlockHeader> headers{bh1, bh2};
  std::vector<BlockHeader> headers_reverse{bh2, bh1};

  if (ticket_hash_2 < ticket_hash_1) {
    cids.swap(cids_reverse);
    headers.swap(headers_reverse);
  }

  ASSERT_EQ(ts.key.cids(), cids);
  ASSERT_EQ(ts.height(), bh1.height);
  ASSERT_EQ(ts.blks, headers);
  ASSERT_EQ(ts.getMinTimestamp(), 7u);
  ASSERT_EQ(ts.getMinTicketBlock(), bh1);
  ASSERT_EQ(ts.getParentStateRoot(), parent_state_root);
  ASSERT_EQ(ts.getParentWeight(), parent_weight);
  ASSERT_TRUE(ts.contains(cid1));
  ASSERT_TRUE(ts.contains(cid2));
  ASSERT_FALSE(ts.contains(cid3));

  EXPECT_OUTCOME_TRUE(tipset2, Tipset::create({bh2, bh1}));
  const auto &ts2 = *tipset2;
  ASSERT_EQ(ts.key.cids(), ts2.key.cids());
  ASSERT_EQ(ts.blks, ts2.blks);
}
