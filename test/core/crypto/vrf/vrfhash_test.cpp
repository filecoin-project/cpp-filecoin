/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/vrf/vrfhash_provider.hpp"

#include <gtest/gtest.h>
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"
#include "testutil/primitives/address_utils.hpp"

using fc::common::Blob;
using fc::common::Buffer;
using fc::crypto::randomness::DomainSeparationTag;
using fc::crypto::vrf::VRFHash;
using fc::crypto::vrf::VRFHashProvider;
using fc::primitives::address::Address;

struct VRFHashProviderTest : public ::testing::Test {
  VRFHashProvider hash_provider{};
  Buffer message{"a1b2c3"_unhex};
  Blob<48> bls_blob =
      "1234567890123456789012345678901234567890"
      "1234567890123456789012345678901234567890"
      "1122334455667788"_blob48;
  Address id_address;
  Address bls_address;
  VRFHash vrf_hash;

  void SetUp() override {
    id_address = makeIdAddress(123u);
    bls_address = makeBlsAddress(bls_blob);
    vrf_hash =
        "661E466606D72B22721484220DCF3FFB44A3ACA3A5D2CC883C9B26281C8E8B27"_blob32;
  }
};

/**
 * @given vrf hash provider, id-based address, DST-tag value and some message
 * @when hash provider method `create` is called
 * @then error is returned
 */
TEST_F(VRFHashProviderTest,
       HashIdAddressFail){EXPECT_OUTCOME_FALSE_1(hash_provider.create(
    DomainSeparationTag::TicketProductionDST, id_address, message))}

/**
 * @given vrf hash provider, bls-based address, DST-tag value and some message
 * @when hash provider method `create` is called
 * @then valid hash is returned
 */
TEST_F(VRFHashProviderTest, HashBlsAddressSuccess) {
  EXPECT_OUTCOME_TRUE(
      result,
      hash_provider.create(
          DomainSeparationTag::TicketProductionDST, bls_address, message))
  ASSERT_EQ(result, vrf_hash);
}
