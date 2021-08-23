/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/types/miner/bitfield_queue.hpp"

#include <gtest/gtest.h>
#include "storage/ipfs/impl/in_memory_datastore.hpp"
#include "testutil/outcome.hpp"

namespace fc::vm::actor::builtin::types::miner {
  using storage::ipfs::InMemoryDatastore;

  struct BitfieldQueueTest : testing::Test {
    void SetUp() override {
      ipld->actor_version = ActorVersion::kVersion0;
      cbor_blake::cbLoadT(ipld, expected);
      cbor_blake::cbLoadT(ipld, queue);
    }

    std::shared_ptr<InMemoryDatastore> ipld{
        std::make_shared<InMemoryDatastore>()};

    BitfieldQueue<storage::amt::kDefaultBits> expected;
    BitfieldQueue<storage::amt::kDefaultBits> queue;
  };

  TEST_F(BitfieldQueueTest, AddValuesToEmptyQueue) {
    const ChainEpoch epoch{42};

    EXPECT_OUTCOME_TRUE_1(expected.queue.set(epoch, {1, 2, 3, 4}));
    expected.quant = kNoQuantization;

    queue.quant = kNoQuantization;

    EXPECT_OUTCOME_TRUE_1(queue.addToQueue(epoch, {1, 2, 3, 4}));

    EXPECT_OUTCOME_TRUE(expected_values, expected.queue.values());
    EXPECT_OUTCOME_TRUE(queue_values, queue.queue.values());

    EXPECT_EQ(queue_values, expected_values);
  }

  TEST_F(BitfieldQueueTest, AddValuesToQuantizedQueue) {
    const std::vector<ChainEpoch> values{0, 2, 3, 4, 7, 8, 9};

    EXPECT_OUTCOME_TRUE_1(expected.queue.set(3, {0, 2, 3}));
    EXPECT_OUTCOME_TRUE_1(expected.queue.set(8, {4, 7, 8}));
    EXPECT_OUTCOME_TRUE_1(expected.queue.set(13, {9}));
    expected.quant = kNoQuantization;

    queue.quant = QuantSpec(5, 3);

    for (auto value : values) {
      EXPECT_OUTCOME_TRUE_1(
          queue.addToQueue(value, {static_cast<uint64_t>(value)}));
    }

    EXPECT_OUTCOME_TRUE(expected_values, expected.queue.values());
    EXPECT_OUTCOME_TRUE(queue_values, queue.queue.values());

    EXPECT_EQ(queue_values, expected_values);
  }

  TEST_F(BitfieldQueueTest, MergeValuesWithSameEpoch) {
    const ChainEpoch epoch{42};

    EXPECT_OUTCOME_TRUE_1(expected.queue.set(epoch, {1, 2, 3, 4}));
    expected.quant = kNoQuantization;

    queue.quant = kNoQuantization;

    EXPECT_OUTCOME_TRUE_1(queue.addToQueue(epoch, {1, 2}));
    EXPECT_OUTCOME_TRUE_1(queue.addToQueue(epoch, {3, 4}));

    EXPECT_OUTCOME_TRUE(expected_values, expected.queue.values());
    EXPECT_OUTCOME_TRUE(queue_values, queue.queue.values());

    EXPECT_EQ(queue_values, expected_values);
  }

  TEST_F(BitfieldQueueTest, AddValuesWithDifferentEpochs) {
    queue.quant = kNoQuantization;

    const ChainEpoch epoch1{42};
    const ChainEpoch epoch2{93};

    const RleBitset expected_values_epoch1{1, 3};
    const RleBitset expected_values_epoch2{2, 4};

    EXPECT_OUTCOME_TRUE_1(queue.addToQueue(epoch1, {1, 3}));
    EXPECT_OUTCOME_TRUE_1(queue.addToQueue(epoch2, {2, 4}));

    EXPECT_OUTCOME_EQ(queue.queue.get(epoch1), expected_values_epoch1);
    EXPECT_OUTCOME_EQ(queue.queue.get(epoch2), expected_values_epoch2);
  }

  TEST_F(BitfieldQueueTest, PopUntilFromEmptyQueue) {
    queue.quant = kNoQuantization;

    EXPECT_OUTCOME_TRUE(result, queue.popUntil(42));
    auto &[next, modified] = result;

    EXPECT_EQ(modified, false);
    EXPECT_EQ(next.empty(), true);
  }

  TEST_F(BitfieldQueueTest, PopUntilBeforeFirstValue) {
    queue.quant = kNoQuantization;

    const ChainEpoch epoch1{42};
    const ChainEpoch epoch2{93};

    EXPECT_OUTCOME_TRUE_1(queue.addToQueue(epoch1, {1, 3}));
    EXPECT_OUTCOME_TRUE_1(queue.addToQueue(epoch2, {2, 4}));

    EXPECT_OUTCOME_TRUE(result, queue.popUntil(epoch1 - 1));
    auto &[next, modified] = result;

    EXPECT_EQ(modified, false);
    EXPECT_EQ(next.empty(), true);
  }

  TEST_F(BitfieldQueueTest, PopUntilSuccess) {
    queue.quant = kNoQuantization;

    const ChainEpoch epoch1{42};
    const ChainEpoch epoch2{93};
    const ChainEpoch epoch3{94};
    const ChainEpoch epoch4{203};

    EXPECT_OUTCOME_TRUE_1(queue.addToQueue(epoch1, {1, 3}));
    EXPECT_OUTCOME_TRUE_1(queue.addToQueue(epoch2, {5}));
    EXPECT_OUTCOME_TRUE_1(queue.addToQueue(epoch3, {6, 7, 8}));
    EXPECT_OUTCOME_TRUE_1(queue.addToQueue(epoch4, {2, 4}));

    RleBitset next;
    bool modified = false;

    EXPECT_OUTCOME_TRUE(result1, queue.popUntil(epoch2));
    std::tie(next, modified) = result1;

    RleBitset expect_bitset;
    expect_bitset = {1, 3, 5};

    EXPECT_EQ(modified, true);
    EXPECT_EQ(next, expect_bitset);

    const RleBitset expected_values_epoch3{6, 7, 8};
    const RleBitset expected_values_epoch4{2, 4};

    EXPECT_OUTCOME_EQ(queue.queue.tryGet(epoch1), boost::none);
    EXPECT_OUTCOME_EQ(queue.queue.tryGet(epoch2), boost::none);
    EXPECT_OUTCOME_EQ(queue.queue.get(epoch3), expected_values_epoch3);
    EXPECT_OUTCOME_EQ(queue.queue.get(epoch4), expected_values_epoch4);

    EXPECT_OUTCOME_TRUE(result2, queue.popUntil(epoch3 - 1));
    std::tie(next, modified) = result2;

    EXPECT_EQ(modified, false);
    EXPECT_EQ(next.empty(), true);

    EXPECT_OUTCOME_EQ(queue.queue.tryGet(epoch1), boost::none);
    EXPECT_OUTCOME_EQ(queue.queue.tryGet(epoch2), boost::none);
    EXPECT_OUTCOME_EQ(queue.queue.get(epoch3), expected_values_epoch3);
    EXPECT_OUTCOME_EQ(queue.queue.get(epoch4), expected_values_epoch4);

    EXPECT_OUTCOME_TRUE(result3, queue.popUntil(epoch4));
    std::tie(next, modified) = result3;

    expect_bitset = {2, 4, 6, 7, 8};

    EXPECT_EQ(modified, true);
    EXPECT_EQ(next, expect_bitset);

    EXPECT_OUTCOME_TRUE(values, queue.queue.values());
    EXPECT_EQ(values.empty(), true);
  }

  TEST_F(BitfieldQueueTest, CutElements) {
    queue.quant = kNoQuantization;

    const ChainEpoch epoch1{42};
    const ChainEpoch epoch2{93};

    const RleBitset expected_values_epoch1{1, 2, 95};

    EXPECT_OUTCOME_TRUE_1(queue.addToQueue(epoch1, {1, 2, 3, 4, 99}));
    EXPECT_OUTCOME_TRUE_1(queue.addToQueue(epoch2, {5, 6}));

    EXPECT_OUTCOME_TRUE_1(queue.cut({2, 4, 5, 6}));

    EXPECT_OUTCOME_EQ(queue.queue.get(epoch1), expected_values_epoch1);
    EXPECT_OUTCOME_EQ(queue.queue.tryGet(epoch2), boost::none);
  }

}  // namespace fc::vm::actor::builtin::types::miner
