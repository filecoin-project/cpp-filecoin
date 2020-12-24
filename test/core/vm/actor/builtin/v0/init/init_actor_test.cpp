/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v0/init/init_actor.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "adt/address_key.hpp"
#include "storage/ipfs/impl/in_memory_datastore.hpp"
#include "testutil/cbor.hpp"
#include "testutil/mocks/vm/runtime/runtime_mock.hpp"

namespace fc::vm::actor::builtin::v0::init {
  using common::Buffer;
  using message::UnsignedMessage;
  using primitives::BigInt;
  using primitives::address::Address;
  using runtime::MockRuntime;
  using storage::ipfs::InMemoryDatastore;

  struct InitActorTest : testing::Test {
    void SetUp() override {
      EXPECT_CALL(runtime, getIpfsDatastore())
          .Times(testing::AnyNumber())
          .WillRepeatedly(testing::Return(ipld));

      EXPECT_CALL(runtime, getImmediateCaller())
          .Times(testing::AnyNumber())
          .WillRepeatedly(testing::Invoke([&]() { return caller; }));

      EXPECT_CALL(runtime, commit(testing::_))
          .Times(testing::AtMost(1))
          .WillOnce(testing::Invoke([&](auto &cid) {
            EXPECT_OUTCOME_TRUE(new_state, ipld->getCbor<InitActorState>(cid));
            state = std::move(new_state);
            return fc::outcome::success();
          }));
    }

    const std::string network_name = "test_network_name";

    MockRuntime runtime;
    std::shared_ptr<InMemoryDatastore> ipld{
        std::make_shared<InMemoryDatastore>()};
    Address caller;
    InitActorState state;
  };

  /** Init actor state CBOR encoding and decoding */
  TEST_F(InitActorTest, InitActorStateCbor) {
    InitActorState init_actor_state{{"010001020000"_cid}, 3, "n"};
    expectEncodeAndReencode(init_actor_state,
                            "83d82a470001000102000003616e"_unhex);
  }

  /// Init actor exec params CBOR encoding and decoding
  TEST_F(InitActorTest, InitActorExecParamsCbor) {
    Exec::Params params{CodeId{"010001020000"_cid}, MethodParams{"de"_unhex}};
    expectEncodeAndReencode(params, "82d82a470001000102000041de"_unhex);
  }

  /**
   * @given caller is system actor
   * @when construct is called
   * @then init actor is constructed
   */
  TEST_F(InitActorTest, ConstructSuccess) {
    caller = kSystemActorAddress;

    EXPECT_OUTCOME_TRUE_1(Construct::call(runtime, {network_name}));

    EXPECT_OUTCOME_TRUE(keys, state.address_map.keys());
    EXPECT_TRUE(keys.empty());
    EXPECT_EQ(state.next_id, 0);
    EXPECT_EQ(state.network_name, network_name);
  }

  /**
   * @given Init actor state and actor address
   * @when Add actor address
   * @then Actor address is mapped to id
   */
  TEST_F(InitActorTest, AddActor) {
    state = {{ipld}, 3, network_name};
    const Address address{primitives::address::ActorExecHash{}};
    const auto expected = Address::makeFromId(state.next_id);

    EXPECT_OUTCOME_EQ(state.addActor(address), expected);

    EXPECT_EQ(state.next_id, 4);
    EXPECT_OUTCOME_EQ(state.address_map.get(address), 3);
  }
};  // namespace fc::vm::actor::builtin::v0::init
