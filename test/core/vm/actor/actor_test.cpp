/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/actor.hpp"

#include "codec/cbor/cbor.hpp"
#include <gtest/gtest.h>
#include "testutil/literals.hpp"

using libp2p::multi::ContentIdentifier;

void expectCidEq(const ContentIdentifier &actual, const std::vector<uint8_t> &expected) {
  EXPECT_EQ(libp2p::multi::ContentIdentifierCodec::encode(actual).value(), expected);
}

/**
 * @given Go constants
 * @when Compare to C++ constants
 * @then Equal
 */
TEST(ActorTest, CidGoCompatibility) {
  expectCidEq(fc::vm::actor::kAccountCodeCid, "0155000d66696c2f312f6163636f756e74"_unhex);
  expectCidEq(fc::vm::actor::kCronCodeCid, "0155000a66696c2f312f63726f6e"_unhex);
  expectCidEq(fc::vm::actor::kStoragePowerCodeCid, "0155000b66696c2f312f706f776572"_unhex);
  expectCidEq(fc::vm::actor::kStorageMarketCodeCid, "0155000c66696c2f312f6d61726b6574"_unhex);
  expectCidEq(fc::vm::actor::kStorageMinerCodeCid, "0155000b66696c2f312f6d696e6572"_unhex);
  expectCidEq(fc::vm::actor::kMultisigCodeCid, "0155000e66696c2f312f6d756c7469736967"_unhex);
  expectCidEq(fc::vm::actor::kInitCodeCid, "0155000a66696c2f312f696e6974"_unhex);
  expectCidEq(fc::vm::actor::kPaymentChannelCodeCid, "0155000b66696c2f312f7061796368"_unhex);

  expectCidEq(fc::vm::actor::kEmptyObjectCid, "01711220c19a797fa1fd590cd2e5b42d1cf5f246e29b91684e2f87404b81dc345c7a56a0"_unhex);
}

/** Actor CBOR encoding and decoding */
TEST(ActorTest, ActorGoCompatibility) {
  auto go = "84d82a52000155000d66696c2f312f6163636f756e74d82a4f000155000a66696c2f312f63726f6e03420005"_unhex;
  fc::vm::actor::Actor actor{fc::vm::actor::kEmptyObjectCid, fc::vm::actor::kEmptyObjectCid, 0, 0};

  fc::codec::cbor::CborDecodeStream(go) >> actor;
  EXPECT_EQ((fc::codec::cbor::CborEncodeStream() << actor).data(), go);

  EXPECT_EQ(actor.code, fc::vm::actor::kAccountCodeCid);
  EXPECT_EQ(actor.head, fc::vm::actor::kCronCodeCid);
  EXPECT_EQ(actor.nonce, 3);
  EXPECT_EQ(actor.balance, 5);
}
