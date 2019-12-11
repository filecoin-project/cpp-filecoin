/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "actor/actor.hpp"

#include <gtest/gtest.h>
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"

using libp2p::multi::ContentIdentifier;

void expectCidEq(const ContentIdentifier &actual, const std::vector<uint8_t> &expected) {
  EXPECT_EQ(libp2p::multi::ContentIdentifierCodec::encode(actual).value(), expected);
}

/**
 * @given Go constants
 * @when Compare to C++ constants
 * @then Equal
 */
TEST(Actor, CidGoCompatibility) {
  expectCidEq(fc::actor::kAccountCodeCid, "0155000d66696c2f312f6163636f756e74"_unhex);
  expectCidEq(fc::actor::kCronCodeCid, "0155000a66696c2f312f63726f6e"_unhex);
  expectCidEq(fc::actor::kStoragePowerCodeCid, "0155000b66696c2f312f706f776572"_unhex);
  expectCidEq(fc::actor::kStorageMarketCodeCid, "0155000c66696c2f312f6d61726b6574"_unhex);
  expectCidEq(fc::actor::kStorageMinerCodeCid, "0155000b66696c2f312f6d696e6572"_unhex);
  expectCidEq(fc::actor::kMultisigCodeCid, "0155000e66696c2f312f6d756c7469736967"_unhex);
  expectCidEq(fc::actor::kInitCodeCid, "0155000a66696c2f312f696e6974"_unhex);
  expectCidEq(fc::actor::kPaymentChannelCodeCid, "0155000b66696c2f312f7061796368"_unhex);

  expectCidEq(fc::actor::kEmptyObjectCid, "01711220c19a797fa1fd590cd2e5b42d1cf5f246e29b91684e2f87404b81dc345c7a56a0"_unhex);
}
