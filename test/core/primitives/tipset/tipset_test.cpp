/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/tipset/tipset.hpp"

#include <gtest/gtest.h>
#include "common/hexutil.hpp"
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
  using Ticket = fc::primitives::ticket::Ticket;
  using Tipset = fc::primitives::tipset::Tipset;
  using TipsetError = fc::primitives::tipset::TipsetError;

  BlockHeader makeBlock() {
    auto bls1 =
        "010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101"_blob96;
    auto bls2 =
        "020101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101"_blob96;

    ticket1 = Ticket{bls1};
    ticket2 = Ticket{bls2};

    BlockHeader block_header = fc::primitives::block::BlockHeader{
        fc::primitives::address::Address::makeFromId(1),
        ticket2,
        {
            {fc::primitives::sector::PoStProof{
                fc::primitives::sector::RegisteredProof::StackedDRG2KiBSeal,
                "F00D"_unhex,
            }},
            bls1,
            {},
        },
        {"010001020002"_cid},
        BigInt(3),
        4,
        "010001020005"_cid,
        "010001020006"_cid,
        "010001020007"_cid,
        Signature{kSampleBlsSignatureBytes},
        8,
        Signature{kSampleBlsSignatureBytes},
        9,
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

    tipset_cbor_hex =
        "8381D82A5827000171A0E40220D820E1A8ED5635DC4A023F27F85FB6EC5968059F711B"
        "D8AA27633D24EDFAFA09818D4200018158600201010101010101010101010101010101"
        "0101010101010101010101010101010101010101010101010101010101010101010101"
        "0101010101010101010101010101010101010101010101010101010101010101010101"
        "0101010101010101018381820342F00D58600101010101010101010101010101010101"
        "0101010101010101010101010101010101010101010101010101010101010101010101"
        "0101010101010101010101010101010101010101010101010101010101010101010101"
        "0101010101010101018081D82A470001000102000242000304D82A4700010001020005"
        "D82A4700010001020006D82A4700010001020007586102010101010101010101010101"
        "0101010101010101010101010101010101010101010101010101010101010101010101"
        "0101010101010101010101010101010101010101010101010101010101010101010101"
        "0101010101010101010101010101085861020101010101010101010101010101010101"
        "0101010101010101010101010101010101010101010101010101010101010101010101"
        "0101010101010101010101010101010101010101010101010101010101010101010101"
        "0101010101010101010904";
  }

  BlockHeader bh1, bh2, bh3, bh4;
  CID cid1, cid2, cid3;
  CID parent_state_root;
  Ticket ticket1, ticket2;
  Signature signature;
  BigInt parent_weight;
  std::string tipset_cbor_hex;
};

/**
 * @given empty set of blockheaders
 * @when create tipset based on that headers
 * @then creation fails
 */
TEST_F(TipsetTest, CreateEmptyFailure) {
  EXPECT_OUTCOME_FALSE(err, Tipset::create({}));
  ASSERT_EQ(err, TipsetError::NO_BLOCKS);
}

/**
 * @given invalid set of blockheaders, where parents heights don't match
 * @when create tipset based on that headers
 * @then creation fails
 */
TEST_F(TipsetTest, CreateMismatchingHeightsFailure) {
  EXPECT_OUTCOME_FALSE(err, Tipset::create({bh1, bh3}));
  ASSERT_EQ(err, TipsetError::MISMATCHING_HEIGHTS);
}

/**
 * @given invalid set of blockheaders, where parents are not equal
 * @when create tipset based on that headers
 * @then creation fails
 */
TEST_F(TipsetTest, CreateMismatchingParentsFailure) {
  EXPECT_OUTCOME_FALSE(err, Tipset::create({bh1, bh4}));
  ASSERT_EQ(err, TipsetError::MISMATCHING_PARENTS);
}

/**
 * @given valid set of blockheaders
 * @when create tipset based on that headers
 * @then creation succeeds and methods return expected values
 */
TEST_F(TipsetTest, CreateSuccess) {
  EXPECT_OUTCOME_TRUE(ts, Tipset::create({bh1, bh2}));
  std::vector<CID> cids{cid2, cid1};
  ASSERT_EQ(ts.cids, cids);
  std::vector<BlockHeader> headers{bh2, bh1};
  ASSERT_EQ(ts.height, bh1.height);
  ASSERT_EQ(ts.blks, headers);
  ASSERT_EQ(ts.getMinTimestamp(), 7u);
  ASSERT_EQ(ts.getMinTicketBlock(), bh2);
  ASSERT_EQ(ts.getParentStateRoot(), parent_state_root);
  ASSERT_EQ(ts.getParentWeight(), parent_weight);
  ASSERT_TRUE(ts.contains(cid1));
  ASSERT_TRUE(ts.contains(cid2));
  ASSERT_FALSE(ts.contains(cid3));
}

/**
 * @given tipset and its serialized representation from go
 * @when encode @and decode the tipset
 * @then decoded version matches the original @and encoded matches the go ones
 */
TEST_F(TipsetTest, LotusCrossTestSuccess) {
  EXPECT_OUTCOME_TRUE(ts, Tipset::create({bh1}));
  EXPECT_OUTCOME_TRUE(tipset_cbor_bytes, fc::common::unhex(tipset_cbor_hex));
  expectEncodeAndReencode(ts, tipset_cbor_bytes);
}
