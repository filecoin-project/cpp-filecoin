/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/impl/paych_voucher.hpp"

#include <gtest/gtest.h>

#include "storage/in_memory/in_memory_storage.hpp"
#include "storage/ipfs/impl/in_memory_datastore.hpp"
#include "testutil/mocks/api.hpp"
#include "testutil/outcome.hpp"

namespace fc::paych_vouchers {
  struct Test : ::testing::Test {
    IpldPtr ipld{std::make_shared<storage::ipfs::InMemoryDatastore>()};
    ApiPtr api{std::make_shared<api::FullNodeApi>()};
    MapPtr kv{std::make_shared<storage::InMemoryStorage>()};
    std::shared_ptr<PaychVouchers> vouchers{
        std::make_shared<PaychVouchers>(ipld, api, kv)};

    const api::NetworkVersion network{api::NetworkVersion::kVersion14};
    const Address paych{Address::makeActorExec({})};
    TokenAmount balance;
    PaymentChannelActorStatePtr state{actorVersion(network)};

    MOCK_API(api, ChainHead);
    MOCK_API(api, StateGetActor);
    MOCK_API(api, StateNetworkVersion);
    MOCK_API(api, WalletSign);
    MOCK_API(api, WalletVerify);

    void SetUp() override {
      api::implPaychVoucher(api, vouchers);

      ipld->actor_version = state.actor_version;
      state.load(ipld);

      api::BlockHeader block;
      block.parent_state_root = block.parent_message_receipts = block.messages =
          CID{CbCid{}};
      block.ticket.emplace();
      block.height = 100;
      const auto head{primitives::tipset::Tipset::create({block}).value()};
      EXPECT_CALL(mock_ChainHead, Call()).WillRepeatedly(testing::Return(head));
      EXPECT_CALL(mock_StateGetActor, Call(paych, head->key))
          .WillRepeatedly(testing::Invoke([=](auto &&, auto &&) {
            api::Actor actor;
            actor.head = setCbor(ipld, state).value();
            actor.balance = balance;
            return actor;
          }));
      EXPECT_CALL(mock_StateNetworkVersion, Call(head->key))
          .WillRepeatedly(testing::Return(network));
      api::Signature sig;
      state->from = Address::makeBls({});
      EXPECT_CALL(mock_WalletSign, Call(state->from, testing::_))
          .WillRepeatedly(testing::Return(sig));
      EXPECT_CALL(mock_WalletVerify, Call(state->from, testing::_, sig))
          .WillRepeatedly(testing::Return(true));
    }

    LaneId nextLaneIs(LaneId expected) {
      EXPECT_OUTCOME_EQ(api->PaychAllocateLane(paych), expected);
      return expected;
    }

    void makeFails(LaneId lane, const TokenAmount &amount) {
      EXPECT_OUTCOME_FALSE_1(api->PaychVoucherCreate(paych, amount, lane));
    }

    SignedVoucher makeOk(LaneId lane, const TokenAmount &amount) {
      EXPECT_OUTCOME_TRUE(voucher,
                          api->PaychVoucherCreate(paych, amount, lane));
      return voucher;
    }

    void checkFails(const SignedVoucher &voucher) {
      EXPECT_OUTCOME_FALSE_1(api->PaychVoucherCheckValid(paych, voucher));
    }

    void checkOk(const SignedVoucher &voucher) {
      EXPECT_OUTCOME_TRUE_1(api->PaychVoucherCheckValid(paych, voucher));
    }

    void addFails(const SignedVoucher &voucher, const TokenAmount &min_delta) {
      EXPECT_OUTCOME_FALSE_1(
          api->PaychVoucherAdd(paych, voucher, {}, min_delta));
    }

    TokenAmount addOk(const SignedVoucher &voucher,
                      const TokenAmount &min_delta) {
      EXPECT_OUTCOME_TRUE(delta,
                          api->PaychVoucherAdd(paych, voucher, {}, min_delta));
      return delta;
    }
  };

  TEST_F(Test, LaneIncrements) {
    nextLaneIs(0);
    nextLaneIs(1);
    nextLaneIs(2);
  }

  TEST_F(Test, LaneFromVoucher) {
    nextLaneIs(0);
    const auto lane100{100};
    balance = 100;
    makeOk(lane100, balance);
    nextLaneIs(lane100 + 1);
  }

  TEST_F(Test, Flow) {
    balance = 100;
    auto voucher{makeOk(0, balance - 1)};
    checkOk(voucher);
    addFails(voucher, voucher.amount + 1);
    EXPECT_EQ(addOk(voucher, voucher.amount), voucher.amount);
    checkFails(voucher);
    EXPECT_EQ(addOk(voucher, voucher.amount), 0);
    makeFails(1, balance - voucher.amount + 1);
  }
}  // namespace fc::paych_vouchers
