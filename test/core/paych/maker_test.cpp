/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/impl/paych_get.hpp"

#include <gtest/gtest.h>

#include "storage/in_memory/in_memory_storage.hpp"
#include "testutil/default_print.hpp"
#include "testutil/mocks/api.hpp"
#include "testutil/outcome.hpp"
#include "vm/actor/builtin/v0/init/init_actor.hpp"

namespace fc::paych_maker {
  using testing::_;
  using vm::actor::builtin::v0::init::Exec;

  struct Test : ::testing::Test {
    ApiPtr api{std::make_shared<api::FullNodeApi>()};
    MapPtr kv{std::make_shared<storage::InMemoryStorage>()};
    std::shared_ptr<PaychMaker> maker{std::make_shared<PaychMaker>(api, kv)};

    const api::NetworkVersion network{api::NetworkVersion::kVersion14};
    const Address from{Address::makeBls({})};
    const Address to{Address::makeSecp256k1({})};

    MOCK_API(api, MpoolPushMessage);
    MOCK_API(api, StateNetworkVersion);
    MOCK_API(api, StateWaitMsg);

    void SetUp() override {
      api::fillPaychGet(api, maker);
    }
    void TearDown() override {
      *api = {};
    }
  };

  /// Create one payment channel
  TEST_F(Test, Create) {
    const TokenAmount amount{10};
    EXPECT_CALL(mock_StateNetworkVersion, Call(api::TipsetKey{}))
        .WillOnce(testing::Return(network));
    boost::optional<CID> cid;
    EXPECT_CALL(mock_MpoolPushMessage, Call(_, _))
        .WillOnce(testing::Invoke([&](auto &msg, auto &&) {
          EXPECT_EQ(msg.from, from);
          EXPECT_EQ(msg.to, vm::actor::kInitAddress);
          EXPECT_EQ(msg.value, amount);
          SignedMessage smsg{msg, {}};
          cid = smsg.getCid();
          return smsg;
        }));
    const auto paych{Address::makeActorExec({})};
    EXPECT_CALL(mock_StateWaitMsg, Call(_, _, _, _))
        .WillOnce(testing::Invoke([&](auto &msg, auto &&, auto &&, auto &&) {
          EXPECT_EQ(msg, *cid);
          MsgWait wait;
          wait.receipt.return_value =
              codec::cbor::encode(Exec::Result{Address::makeFromId(100), paych})
                  .value();
          return wait;
        }));
    EXPECT_OUTCOME_TRUE(info, api->PaychGet(from, to, amount));
    EXPECT_EQ(info.channel, paych);
    EXPECT_EQ(info.channel_message, *cid);
  }
}  // namespace fc::paych_maker
