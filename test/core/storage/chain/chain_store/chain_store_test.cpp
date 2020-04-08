/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/chain/impl/chain_store_impl.hpp"

#include <gtest/gtest.h>
#include "blockchain/impl/weight_calculator_impl.hpp"
#include "common/hexutil.hpp"
#include "primitives/cid/cid_of_cbor.hpp"
#include "storage/chain/impl/chain_data_store_impl.hpp"
#include "storage/ipfs/impl/in_memory_datastore.hpp"
#include "storage/ipfs/impl/ipfs_block_service.hpp"
#include "testutil/literals.hpp"
#include "testutil/mocks/blockchain/block_validator/block_validator_mock.hpp"
#include "testutil/mocks/blockchain/weight_calculator_mock.hpp"
#include "testutil/outcome.hpp"

using fc::blockchain::block_validator::BlockValidatorMock;
using fc::blockchain::weight::WeightCalculatorImpl;
using fc::blockchain::weight::WeightCalculatorMock;
using fc::crypto::signature::Signature;
using fc::primitives::BigInt;
using fc::primitives::block::BlockHeader;
using fc::primitives::ticket::Ticket;
using fc::storage::blockchain::ChainDataStoreImpl;
using fc::storage::blockchain::ChainStoreImpl;
using fc::storage::ipfs::InMemoryDatastore;
using fc::storage::ipfs::IpfsBlockService;

using fc::primitives::cid::getCidOfCbor;
using testing::_;

struct ChainStoreTest : public ::testing::Test {
  BlockHeader makeBlock() {
    auto bls1 =
        "010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101"_blob96;
    auto bls2 =
        "020101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101"_blob96;

    auto &&ticket = Ticket{bls2};

    BlockHeader block_header = fc::primitives::block::BlockHeader{
        fc::primitives::address::Address::makeFromId(1),
        ticket,
        {
            {fc::primitives::sector::PoStProof{
                fc::primitives::sector::RegisteredProof::StackedDRG2KiBSeal,
                "DEAD"_unhex,
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
        "CAFE"_unhex,
        8,
        Signature{"DEAD"_unhex},
        9};
    return block_header;
  }

  void SetUp() override {
    // create chain store
    auto block_service = std::make_shared<IpfsBlockService>(
        std::make_shared<InMemoryDatastore>());
    auto data_store = std::make_shared<ChainDataStoreImpl>(
        std::make_shared<InMemoryDatastore>());
    auto block_validator = std::make_shared<BlockValidatorMock>();
    auto weight_calculator = std::make_shared<WeightCalculatorMock>();

    EXPECT_CALL(*weight_calculator, calculateWeight(testing::_))
        .WillRepeatedly(testing::Return(1));

    EXPECT_OUTCOME_TRUE(store,
                        ChainStoreImpl::create(std::move(block_service),
                                               std::move(data_store),
                                               std::move(block_validator),
                                               std::move(weight_calculator)));
    chain_store = std::move(store);

    block = makeBlock();
  }

  std::shared_ptr<ChainStoreImpl> chain_store;
  BlockHeader block;
};

/**
 * @given chain store, a block
 * @when add block to store
 * @then store contains it
 */
TEST_F(ChainStoreTest, AddBlockSuccess) {
  EXPECT_OUTCOME_TRUE(block_cid, getCidOfCbor(block));
  // doesn't have yet
  EXPECT_OUTCOME_FALSE_1(chain_store->getBlock(block_cid));
  // add
  EXPECT_OUTCOME_TRUE_1(chain_store->addBlock(block));
  // now it has block
  EXPECT_OUTCOME_TRUE(stored_block, chain_store->getBlock(block_cid));
  ASSERT_EQ(block, stored_block);
}
