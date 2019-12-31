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
using fc::crypto::bls::impl::BlsProviderImpl;
using fc::crypto::randomness::DomainSeparationTag;
using fc::crypto::vrf::VRFHash;
using fc::crypto::vrf::VRFHashProvider;
using fc::crypto::vrf::VRFProvider;
using fc::crypto::vrf::VRFProviderImpl;
using fc::crypto::vrf::VRFPublicKey;
using fc::crypto::vrf::VRFSecretKey;
using fc::primitives::address::Address;

struct VRFProviderTest : public ::testing::Test {
  void SetUp() override {
    bls_provider = std::make_shared<BlsProviderImpl>();
    vrf_provider = std::make_shared<VRFProviderImpl>(bls_provider);
  }
  std::shared_ptr<BlsProvider> bls_provider;
  std::shared_ptr<VRFProvider> vrf_provider;
  VRFHash message{
      "661E466606D72B22721484220DCF3FFB44A3ACA3A5D2CC883C9B26281C8E8B27"_blob32};
  VRFHash wrong_message{
      "761E466606D72B22721484220DCF3FFB44A3ACA3A5D2CC883C9B26281C8E8B27"_blob32};
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
                      vrf_provider->generateVRF(vrf_secret_key, message))
  VRFPublicKey vrf_public_key{key_pair.public_key};
  EXPECT_OUTCOME_TRUE(
      signature_status,
      vrf_provider->verifyVRF(vrf_public_key, message, signature))
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
                      vrf_provider->generateVRF(vrf_secret_key, message))
  // get pk from other pair
  VRFPublicKey vrf_public_key{key_pair1.public_key};
  EXPECT_OUTCOME_TRUE(
      signature_status,
      vrf_provider->verifyVRF(vrf_public_key, message, signature))
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
                      vrf_provider->generateVRF(vrf_secret_key, message))
  // get pk from other pair
  VRFPublicKey vrf_public_key{key_pair.public_key};
  EXPECT_OUTCOME_TRUE(
      signature_status,
      vrf_provider->verifyVRF(vrf_public_key, wrong_message, signature))
  ASSERT_FALSE(signature_status);
}
