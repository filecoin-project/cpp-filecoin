/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/init/init_actor.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "primitives/address/address_codec.hpp"
#include "storage/hamt/hamt.hpp"
#include "storage/ipfs/impl/in_memory_datastore.hpp"
#include "testutil/cbor.hpp"
#include "testutil/init_actor.hpp"
#include "testutil/mocks/vm/runtime/runtime_mock.hpp"

namespace InitActor = fc::vm::actor::builtin::init;

using fc::primitives::BigInt;
using fc::primitives::address::Address;
using fc::vm::VMExitCode;
using fc::vm::actor::CodeId;
using fc::vm::actor::InvocationOutput;
using fc::vm::actor::kInitAddress;
using fc::vm::actor::MethodNumber;
using fc::vm::actor::MethodParams;
using fc::vm::message::UnsignedMessage;
using fc::vm::runtime::MockRuntime;
using InitActor::InitActorState;

/** Init actor state CBOR encoding and decoding */
TEST(InitActorTest, InitActorStateCbor) {
  InitActorState init_actor_state{"010001020000"_cid, 3};
  expectEncodeAndReencode(init_actor_state, "82d82a470001000102000003"_unhex);
}

/// Init actor exec params CBOR encoding and decoding
TEST(InitActorTest, InitActorExecParamsCbor) {
  InitActor::ExecParams params{CodeId{"010001020000"_cid},
                               MethodParams{"de"_unhex}};
  expectEncodeAndReencode(params, "82d82a470001000102000041de"_unhex);
}

/**
 * @given Init actor state and actor address
 * @when Add actor address
 * @then Actor address is mapped to id
 */
TEST(InitActorTest, AddActor) {
  using fc::primitives::address::Address;
  using fc::storage::hamt::Hamt;
  auto store = std::make_shared<fc::storage::ipfs::InMemoryDatastore>();
  EXPECT_OUTCOME_TRUE(empty_map, Hamt(store).flush());
  InitActorState state{empty_map, 3};
  Address address{fc::primitives::address::TESTNET,
                  fc::primitives::address::ActorExecHash{}};
  auto expected = Address::makeFromId(state.next_id);
  EXPECT_OUTCOME_EQ(state.addActor(store, address), expected);
  EXPECT_EQ(state.next_id, 4);
  EXPECT_OUTCOME_EQ(
      Hamt(store, state.address_map)
          .getCbor<uint64_t>(fc::primitives::address::encodeToString(address)),
      3);
}

MethodParams execParams(const fc::CID &code, gsl::span<const uint8_t> params) {
  return MethodParams{
      fc::codec::cbor::encode(
          InitActor::ExecParams{CodeId{code}, MethodParams{params}})
          .value()};
}

/**
 * @given Init actor
 * @when Call exec with singleton @and with non-builtin actor code
 * @then Error
 */
TEST(InitActorExecText, ExecError) {
  MockRuntime runtime;

  EXPECT_OUTCOME_ERROR(
      VMExitCode::INIT_ACTOR_NOT_BUILTIN_ACTOR,
      InitActor::exec({}, runtime, execParams("010001020000"_cid, {})));
  EXPECT_OUTCOME_ERROR(
      VMExitCode::INIT_ACTOR_SINGLETON_ACTOR,
      InitActor::exec(
          {}, runtime, execParams(fc::vm::actor::kInitCodeCid, {})));
}

/**
 * @given Init actor
 * @when Call exec with non-singleton builtin actor code
 * @then Actor created
 */
TEST(InitActorExecText, ExecSuccess) {
  UnsignedMessage message;
  message.from = Address::makeFromId(2);
  message.nonce = 3;
  message.value = 4;
  auto params = MethodParams{"dead"_unhex};
  auto id = 100;
  auto id_address = Address::makeFromId(id);
  auto state_tree = setupInitActor(nullptr, id);
  auto init_actor = state_tree->get(kInitAddress).value();
  auto code = fc::vm::actor::kMultisigCodeCid;
  MockRuntime runtime;

  EXPECT_CALL(runtime, chargeGas(testing::_))
      .WillOnce(testing::Return(fc::outcome::success()));

  EXPECT_CALL(runtime, getMessage())
      .WillOnce(testing::Return(std::cref(message)));

  EXPECT_CALL(runtime, getIpfsDatastore())
      .Times(2)
      .WillRepeatedly(testing::Return(state_tree->getStore()));

  EXPECT_CALL(runtime,
              send(id_address,
                   fc::vm::actor::kConstructorMethodNumber,
                   params,
                   message.value))
      .WillOnce(testing::Return(fc::outcome::success()));

  EXPECT_CALL(runtime, getCurrentActorState())
      .WillOnce(testing::Return(init_actor.head));

  EXPECT_CALL(runtime, commit(testing::_))
      .WillOnce(testing::Invoke([&](auto new_state) {
        init_actor.head = new_state;
        return state_tree->set(kInitAddress, init_actor);
      }));

  EXPECT_CALL(runtime, createActor(id_address, testing::_))
      .WillOnce(testing::Invoke([&](auto address, auto actor) {
        EXPECT_OUTCOME_TRUE_1(state_tree->set(address, actor));
        return fc::outcome::success();
      }));

  EXPECT_OUTCOME_EQ(InitActor::exec({}, runtime, execParams(code, params)),
                    InvocationOutput{fc::common::Buffer{
                        fc::primitives::address::encode(id_address)}});

  EXPECT_OUTCOME_TRUE(address,
                      fc::primitives::address::decode(
                          "02218e62925e4f37b905d355e2cbc2b33cca45b39c"_unhex));
  EXPECT_OUTCOME_EQ(state_tree->lookupId(address), id_address);
  EXPECT_OUTCOME_TRUE(actor, state_tree->get(id_address));
  EXPECT_EQ(actor.code, code);
}
