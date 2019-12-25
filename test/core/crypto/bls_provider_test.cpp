/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/bls/impl/bls_provider_impl.hpp"

#include <vector>
#include <algorithm>

#include <gtest/gtest.h>
#include "testutil/outcome.hpp"

using fc::crypto::bls::impl::BlsProviderImpl;
using fc::crypto::bls::KeyPair;
using fc::crypto::bls::PrivateKey;
using fc::crypto::bls::PublicKey;
using fc::crypto::bls::Signature;

class BlsProviderTest : public ::testing::Test {
 protected:
  BlsProviderImpl provider_;
  std::vector<uint8_t> message_{4, 8, 15, 16, 23, 42};
};

/**
 * @given BLS provider
 * @when Generating new key pair and deriving public key from private
 * @then Derived public key must be the same as generated
 */
TEST_F(BlsProviderTest, PublicKeyDeriveSuccess) {
  EXPECT_OUTCOME_TRUE(key_pair, provider_.generateKeyPair());
  EXPECT_OUTCOME_TRUE(derived_public_key,
                      provider_.derivePublicKey(key_pair.private_key));
  ASSERT_EQ(derived_public_key, key_pair.public_key);
}

/**
 * @given Sample message
 * @when Signing and signature verification
 * @then Generated signature must be valid
 */
TEST_F(BlsProviderTest, VerifySignatureSuccess) {
  EXPECT_OUTCOME_TRUE(key_pair, provider_.generateKeyPair());
  EXPECT_OUTCOME_TRUE(signature,
                      provider_.sign(message_, key_pair.private_key));
  EXPECT_OUTCOME_TRUE(
      signature_status,
      provider_.verifySignature(message_, signature, key_pair.public_key));
  ASSERT_TRUE(signature_status);
}

/**
 * @given Sample message
 * @when Signing and signature verification with different public key
 * @then Signature status for different public key must be invalid
 */
TEST_F(BlsProviderTest, SignatureVerificationFailure) {
  EXPECT_OUTCOME_TRUE(key_pair, provider_.generateKeyPair());
  EXPECT_OUTCOME_TRUE(signature,
                      provider_.sign(message_, key_pair.private_key));
  EXPECT_OUTCOME_TRUE(key_pair_second, provider_.generateKeyPair());
  EXPECT_OUTCOME_TRUE(signature_status,
                      provider_.verifySignature(
                          message_, signature, key_pair_second.public_key));
  ASSERT_FALSE(signature_status);
}

/**
 * @given Sample message
 * @when Signing and signature verification for different message
 * @then Signature status for different message must be invalid
 */
TEST_F(BlsProviderTest, DifferentMessageVerificationFailure) {
  EXPECT_OUTCOME_TRUE(key_pair, provider_.generateKeyPair());
  EXPECT_OUTCOME_TRUE(signature,
                      provider_.sign(message_, key_pair.private_key));
  std::vector<uint8_t> different_message{message_};
  std::reverse(different_message.begin(), different_message.end());
  EXPECT_OUTCOME_TRUE(signature_status,
                      provider_.verifySignature(
                          different_message, signature, key_pair.public_key));
  ASSERT_FALSE(signature_status);
}
