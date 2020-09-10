/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/init/init_actor.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "adt/address_key.hpp"
#include "storage/hamt/hamt.hpp"
#include "storage/ipfs/impl/in_memory_datastore.hpp"
#include "testutil/cbor.hpp"
#include "testutil/init_actor.hpp"
#include "testutil/mocks/vm/runtime/runtime_mock.hpp"

using fc::common::Buffer;
using fc::primitives::BigInt;
using fc::primitives::address::Address;
using fc::vm::VMExitCode;
using fc::vm::actor::CodeId;
using fc::vm::actor::encodeActorReturn;
using fc::vm::actor::InvocationOutput;
using fc::vm::actor::kInitAddress;
using fc::vm::actor::MethodNumber;
using fc::vm::actor::MethodParams;
using fc::vm::actor::builtin::init::Exec;
using fc::vm::actor::builtin::init::InitActorState;
using fc::vm::message::UnsignedMessage;
using fc::vm::runtime::MockRuntime;

/** Init actor state CBOR encoding and decoding */
TEST(InitActorTest, InitActorStateCbor) {
  InitActorState init_actor_state{{"010001020000"_cid}, 3, "n"};
  expectEncodeAndReencode(init_actor_state,
                          "83d82a470001000102000003616e"_unhex);
}

/// Init actor exec params CBOR encoding and decoding
TEST(InitActorTest, InitActorExecParamsCbor) {
  Exec::Params params{CodeId{"010001020000"_cid}, MethodParams{"de"_unhex}};
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
  InitActorState state{{store}, 3, "n"};
  Address address{fc::primitives::address::TESTNET,
                  fc::primitives::address::ActorExecHash{}};
  auto expected = Address::makeFromId(state.next_id);
  EXPECT_OUTCOME_EQ(state.addActor(address), expected);
  EXPECT_EQ(state.next_id, 4);
  EXPECT_OUTCOME_EQ(state.address_map.get(address), 3);
}
