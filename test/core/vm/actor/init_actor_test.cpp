/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/init_actor.hpp"

#include <gtest/gtest.h>
#include "primitives/address/address_codec.hpp"
#include "storage/hamt/hamt.hpp"
#include "storage/ipfs/impl/in_memory_datastore.hpp"
#include "testutil/cbor.hpp"

using fc::vm::actor::InitActorState;

/** Init actor state CBOR */
TEST(InitActorTest, ActorState) {
  InitActorState state{"010001020000"_cid, 3};
  expectEncodeAndReencode(state, "82d82a470001000102000003"_unhex);
}
