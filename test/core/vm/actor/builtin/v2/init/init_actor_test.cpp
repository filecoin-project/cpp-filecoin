/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v2/init/init_actor.hpp"

#include <gmock/gmock.h>

#include "adt/address_key.hpp"
#include "testutil/cbor.hpp"
#include "testutil/vm/actor/builtin/actor_test_fixture.hpp"
#include "vm/actor/builtin/v2/codes.hpp"

namespace fc::vm::actor::builtin::v2::init {
  using actor::kConstructorMethodNumber;
  using common::Buffer;
  using message::UnsignedMessage;
  using primitives::TokenAmount;
  using primitives::address::Address;
  using testutil::vm::actor::builtin::ActorTestFixture;
  using version::kUpgradeBreezeHeight;
  using version::kUpgradeKumquatHeight;

  struct InitActorTest : public ActorTestFixture<InitActorState> {
    void SetUp() override {
      ActorTestFixture<InitActorState>::SetUp();
      ipld->load(state);
      actorVersion = ActorVersion::kVersion2;
    }

    const std::string network_name = "test_network_name";
    CodeId code = kStorageMinerCodeId;
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
    callerIs(kSystemActorAddress);

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

  TEST_F(InitActorTest, CallerIdHasError) {
    callerIs(kSystemActorAddress);

    // Test VM_ASSERT

    currentEpochIs(kUpgradeBreezeHeight);
    EXPECT_OUTCOME_ERROR(asAbort(VMExitCode::kOldErrActorFailure),
                         Exec::call(runtime, {code, {}}));

    currentEpochIs(kUpgradeKumquatHeight);
    EXPECT_OUTCOME_ERROR(asAbort(VMExitCode::kSysErrReserved1),
                         Exec::call(runtime, {code, {}}));
  }

  TEST_F(InitActorTest, CannotExec) {
    callerIs(kSystemActorAddress);
    addressCodeIdIs(kSystemActorAddress, kSystemActorCodeId);

    EXPECT_OUTCOME_ERROR(asAbort(VMExitCode::kErrForbidden),
                         Exec::call(runtime, {code, {}}));
  }

  TEST_F(InitActorTest, ExecSuccess) {
    callerIs(kStoragePowerAddress);
    addressCodeIdIs(kStoragePowerAddress, kStoragePowerCodeId);

    const std::vector<uint8_t> data{'a', 'd', 'd', 'r', 'e', 's', 's'};
    const auto actor_address = Address::makeActorExec(data);
    const auto actor_id_address = Address::makeFromId(state.next_id);

    EXPECT_CALL(runtime, createNewActorAddress())
        .WillRepeatedly(testing::Return(actor_address));
    EXPECT_CALL(runtime, createActor(testing::_, testing::_))
        .WillRepeatedly(testing::Return(outcome::success()));
    EXPECT_CALL(runtime, getMessage())
        .WillRepeatedly(testing::Return(UnsignedMessage{}));
    EXPECT_CALL(runtime,
                send(actor_id_address,
                     kConstructorMethodNumber,
                     MethodParams{},
                     TokenAmount{0}))
        .WillOnce(testing::Return(fc::outcome::success()));

    EXPECT_OUTCOME_TRUE(result, Exec::call(runtime, {code, {}}));

    EXPECT_EQ(result.id_address, actor_id_address);
    EXPECT_EQ(result.robust_address, actor_address);
  }
};  // namespace fc::vm::actor::builtin::2::init
