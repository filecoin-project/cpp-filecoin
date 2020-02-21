/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/chain/chain_store.hpp"

#include <gtest/gtest.h>
#include "blockchain/impl/block_validator_impl.hpp"
#include "blockchain/impl/weight_calculator_impl.hpp"
#include "common/hexutil.hpp"
#include "primitives/cid/cid_of_cbor.hpp"
#include "storage/chain/impl/chain_data_store_impl.hpp"
#include "storage/ipfs/impl/ipfs_block_service.hpp"
#include "storage/ipfs/impl/in_memory_datastore.hpp"
#include "testutil/cbor.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"

using fc::blockchain::block_validator::BlockValidatorImpl;
using fc::blockchain::weight::WeightCalculatorImpl;
using fc::primitives::BigInt;
using fc::primitives::block::BlockHeader;
using fc::primitives::ticket::Ticket;
using fc::storage::blockchain::ChainDataStoreImpl;
using fc::storage::blockchain::ChainStore;
using fc::storage::ipfs::IpfsBlockService;
using fc::storage::ipfs::InMemoryDatastore;

using fc::primitives::cid::getCidOfCbor;

struct ChainStoreTest : public ::testing::Test {
  BlockHeader makeBlock() {
    auto bls1 =
        "010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101"_blob96;
    auto bls2 =
        "020101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101"_blob96;

    auto &&ticket = Ticket{bls2};

    auto &&signature = "01DEAD"_unhex;

    BlockHeader block_header = fc::primitives::block::BlockHeader{
        fc::primitives::address::Address::makeFromId(1),
        ticket,
        {
            fc::common::Buffer("F00D"_unhex),
            bls1,
            {},
        },
        {"010001020002"_cid},
        BigInt(3),
        4,
        "010001020005"_cid,
        "010001020006"_cid,
        "010001020007"_cid,
        "01CAFE"_unhex,
        8,
        signature,
        9};
    return block_header;
  }

  void SetUp() override {
    // create chain store
    auto block_service = std::make_shared<IpfsBlockService>(
        std::make_shared<InMemoryDatastore>());
    auto data_store = std::make_shared<ChainDataStoreImpl>(
        std::make_shared<InMemoryDatastore>());
    auto block_validator = std::make_shared<BlockValidatorImpl>();
    auto weight_calculator = std::make_shared<WeightCalculatorImpl>();

    EXPECT_OUTCOME_TRUE(store,
                        ChainStore::create(std::move(block_service),
                                           std::move(data_store),
                                           std::move(block_validator),
                                           std::move(weight_calculator)));
    chain_store = std::move(store);

    block = makeBlock();
  }

  std::shared_ptr<ChainStore> chain_store;
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
