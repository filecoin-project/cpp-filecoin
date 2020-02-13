/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/vrf/impl/vrf_provider_impl.hpp"

#include <gtest/gtest.h>
#include "crypto/bls/impl/bls_provider_impl.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"

using fc::common::Blob;
using fc::common::Buffer;
using fc::crypto::bls::BlsProvider;
using fc::crypto::bls::BlsProviderImpl;
using fc::crypto::randomness::DomainSeparationTag;
using fc::crypto::vrf::VRFHash;
using fc::crypto::vrf::VRFParams;
using fc::crypto::vrf::VRFProvider;
using fc::crypto::vrf::VRFProviderImpl;
using fc::crypto::vrf::VRFPublicKey;
using fc::crypto::vrf::VRFSecretKey;
using fc::primitives::address::Address;

struct VRFProviderTest : public ::testing::Test {
  void SetUp() override {
    bls_provider = std::make_shared<BlsProviderImpl>();
    vrf_provider = std::make_shared<VRFProviderImpl>(bls_provider);

    Buffer message{"a1b2c3"_unhex};
    Buffer wrong_message{"a1b2c4"_unhex};
    Blob<48> bls_blob =
        "1234567890123456789012345678901234567890"
        "1234567890123456789012345678901234567890"
        "1122334455667788"_blob48;
    vrf_params = VRFParams{
        .personalization_tag = DomainSeparationTag::TicketProductionDST,
        .miner_address = Address::makeBls(bls_blob),
        .message = message};
    wrong_vrf_params = VRFParams{
        .personalization_tag = DomainSeparationTag::TicketProductionDST,
        .miner_address = Address::makeBls(bls_blob),
        .message = wrong_message};
  }
  std::shared_ptr<BlsProvider> bls_provider;
  std::shared_ptr<VRFProvider> vrf_provider;
  VRFParams vrf_params;
  VRFParams wrong_vrf_params;
};

/**
 * @given generated vrf (bls) keypair,
 * @when generateVRF method of vrf provider is called
 * @then valid signature successfully obtained @and verifyVRF succeeds @and
 * status is true
 */
TEST_F(VRFProviderTest, VRFGenerateVerifySuccess) {
  EXPECT_OUTCOME_TRUE(key_pair, bls_provider->generateKeyPair())
  VRFSecretKey vrf_secret_key{key_pair.private_key};
  EXPECT_OUTCOME_TRUE(signature,
                      vrf_provider->computeVRF(vrf_secret_key, vrf_params))
  VRFPublicKey vrf_public_key{key_pair.public_key};
  EXPECT_OUTCOME_TRUE(
      signature_status,
      vrf_provider->verifyVRF(vrf_public_key, vrf_params, signature))
  ASSERT_TRUE(signature_status);
}

/**
 * @given 2 generated vrf (bls) keypairs @and original message
 * @when generateVRF method of vrf provider is called
 * @then valid signature successfully obtained @and verifyVRF succeeds @and
 * status is false
 */
TEST_F(VRFProviderTest, VRFVerifyWrongKeyFail) {
  EXPECT_OUTCOME_TRUE(key_pair, bls_provider->generateKeyPair())
  EXPECT_OUTCOME_TRUE(key_pair1, bls_provider->generateKeyPair())

  VRFSecretKey vrf_secret_key{key_pair.private_key};
  EXPECT_OUTCOME_TRUE(signature,
                      vrf_provider->computeVRF(vrf_secret_key, vrf_params))
  // get pk from other pair
  VRFPublicKey vrf_public_key{key_pair1.public_key};
  EXPECT_OUTCOME_TRUE(
      signature_status,
      vrf_provider->verifyVRF(vrf_public_key, vrf_params, signature))
  ASSERT_FALSE(signature_status);
}

/**
 * @given generated vrf (bls) keypair, original message and corrupted message
 * @when generateVRF method of vrf provider is called
 * @then valid signature successfully obtained @and verifyVRF succeeds @and
 * status is false
 */
TEST_F(VRFProviderTest, VRFVerifyWrongMessageFail) {
  EXPECT_OUTCOME_TRUE(key_pair, bls_provider->generateKeyPair())

  VRFSecretKey vrf_secret_key{key_pair.private_key};
  EXPECT_OUTCOME_TRUE(signature,
                      vrf_provider->computeVRF(vrf_secret_key, vrf_params))
  // get pk from other pair
  VRFPublicKey vrf_public_key{key_pair.public_key};
  EXPECT_OUTCOME_TRUE(
      signature_status,
      vrf_provider->verifyVRF(vrf_public_key, wrong_vrf_params, signature))
  ASSERT_FALSE(signature_status);
}
