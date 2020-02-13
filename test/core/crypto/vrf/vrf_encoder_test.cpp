/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/vrf/vrf_hash_encoder.hpp"

#include <gtest/gtest.h>
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"

using fc::common::Blob;
using fc::common::Buffer;
using fc::crypto::randomness::DomainSeparationTag;
using fc::crypto::vrf::encodeVrfParams;
using fc::crypto::vrf::VRFHash;
using fc::crypto::vrf::VRFParams;
using fc::primitives::address::Address;

struct VRFHashEncoderTest : public ::testing::Test {
  Buffer message{"a1b2c3"_unhex};
  Blob<48> bls_blob =
      "1234567890123456789012345678901234567890"
      "1234567890123456789012345678901234567890"
      "1122334455667788"_blob48;
  Address id_address;
  Address bls_address;
  VRFHash vrf_hash;

  void SetUp() override {
    id_address = Address::makeFromId(123u);
    bls_address = Address::makeBls(bls_blob);
    vrf_hash = VRFHash{
        "661E466606D72B22721484220DCF3FFB44A3ACA3A5D2CC883C9B26281C8E8B27"_blob32};
  }
};

/**
 * @given id-based address, DST-tag value and some message
 * @when hash provider method `create` is called
 * @then error is returned
 */
TEST_F(VRFHashEncoderTest,
       HashIdAddressFail){EXPECT_OUTCOME_FALSE_1(encodeVrfParams(VRFParams{
    DomainSeparationTag::TicketProductionDST, id_address, message}))}

/**
 * @given bls-based address, DST-tag value and some message
 * @when encode vrf parameters
 * @then valid hash is returned and its value is correct
 */
TEST_F(VRFHashEncoderTest, HashBlsAddressSuccess) {
  EXPECT_OUTCOME_TRUE(
      result,
      encodeVrfParams(VRFParams{
          DomainSeparationTag::TicketProductionDST, bls_address, message}))
  ASSERT_EQ(result, vrf_hash);
}
