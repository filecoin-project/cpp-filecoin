/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <gtest/gtest.h>
#include "storage/ipfs/impl/in_memory_datastore.hpp"
#include "testutil/mocks/vm/runtime/runtime_mock.hpp"

namespace fc::testutil::vm::actor::builtin::reward {
  using ::fc::vm::actor::kSystemActorAddress;
  using ::fc::vm::runtime::MockRuntime;
  using primitives::ChainEpoch;
  using primitives::StoragePower;
  using primitives::TokenAmount;
  using primitives::address::Address;
  using storage::ipfs::InMemoryDatastore;
  using testing::Eq;
  using testing::Return;

  static const TokenAmount kEpochZeroReward{"36266264293777134739"};

  template <typename State>
  class RewardActorFixture : public testing::Test {
    void SetUp() override {
      EXPECT_CALL(runtime, getIpfsDatastore())
          .Times(testing::AnyNumber())
          .WillRepeatedly(Return(ipld));

      EXPECT_CALL(runtime, getImmediateCaller())
          .Times(testing::AnyNumber())
          .WillRepeatedly(testing::Invoke([&]() { return caller; }));

      EXPECT_CALL(runtime, getCurrentActorState())
          .Times(testing::AnyNumber())
          .WillRepeatedly(testing::Invoke([&]() {
            EXPECT_OUTCOME_TRUE(cid, ipld->setCbor(state));
            return std::move(cid);
          }));

      EXPECT_CALL(runtime, commit(testing::_))
          .Times(testing::AnyNumber())
          .WillRepeatedly(testing::Invoke([&](auto &cid) {
            EXPECT_OUTCOME_TRUE(new_state, ipld->getCbor<State>(cid));
            state = std::move(new_state);
            return fc::outcome::success();
          }));
    }

   public:
    void setCurrentBalance(const TokenAmount &balance) {
      const Address receiver = Address::makeFromId(1001);
      EXPECT_CALL(runtime, getCurrentReceiver())
          .WillRepeatedly(Return(receiver));
      EXPECT_CALL(runtime, getBalance(Eq(receiver)))
          .WillRepeatedly(Return(outcome::success(balance)));
    }

    template <typename Constructor>
    void constructRewardActor(const StoragePower &start_realized_power = 0) {
      caller = kSystemActorAddress;
      EXPECT_OUTCOME_TRUE_1(Constructor::call(runtime, start_realized_power));
    }

    MockRuntime runtime;
    std::shared_ptr<InMemoryDatastore> ipld{
        std::make_shared<InMemoryDatastore>()};
    Address caller;
    State state;
  };
}  // namespace fc::testutil::vm::actor::builtin::reward
