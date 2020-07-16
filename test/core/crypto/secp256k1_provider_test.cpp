/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/secp256k1/impl/secp256k1_provider_impl.hpp"
#include "crypto/secp256k1/impl/secp256k1_sha256_provider_impl.hpp"

#include <gtest/gtest.h>
#include <algorithm>
#include "crypto/secp256k1/secp256k1_error.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"

#define SAMPLE_PRIVATE_KEY_BYTES                                             \
  0xD9, 0x90, 0xE0, 0xF2, 0x4F, 0xFC, 0x86, 0x8C, 0xD6, 0xAC, 0x4D, 0xBA,    \
      0xE1, 0xB3, 0x30, 0x82, 0x31, 0x0, 0xE7, 0x26, 0x75, 0x38, 0x95, 0xC1, \
      0x18, 0x4B, 0x6E, 0xC3, 0x88, 0x50, 0x64, 0xD1

namespace fc::crypto::secp256k1 {

  /**
   * @brief Pre-generated key pair and signature for sample message
   * @using reference implementation from github.com/libp2p/go-libp2p-core
   */

  class Secp256k1ProviderTest : public ::testing::Test {
   protected:
    /**
     * Sample message, signature, and pubkey from go-secp256k1
     * (https://github.com/ipsn/go-secp256k1/blob/master/secp256_test.go#L206)
     * Pay attention that hash function is not used for message (it is already
     * 32 byte long)
     */
    std::vector<uint8_t> go_public_key_bytes{
        "04e32df42865e97135acfb65f3bae71bdc86f4d49150ad6a440b6f15878109880a0a2b2667f7e725ceea70c673093bf67663e0312623c8e091b13cf2c0f11ef652"_unhex};
    std::vector<uint8_t> go_signature_bytes{
        "90f27b8b488db00b00606796d2987f6a5f59ae62ea05effe84fef5b8b0e549984a691139ad57a3f0b906637673aa2f63d1f55cb1a69199d4009eea23ceaddc9301"_unhex};
    std::vector<uint8_t> go_message_hash{
        "ce0677bb30baa8cf067c88db9811f4333d131bf8bcf12fe7065d211dce971008"_unhex};
    SignatureCompact go_signature{};
    PublicKeyUncompressed go_public_key{};

    PrivateKey private_key_{SAMPLE_PRIVATE_KEY_BYTES};
    std::shared_ptr<Secp256k1ProviderDefault> secp256K1_provider =
        std::make_shared<Secp256k1Sha256ProviderImpl>();

    // secp256k1 provider without digest for interoperability test
    std::shared_ptr<Secp256k1ProviderDefault> secp256K1_no_digest_provider =
        std::make_shared<Secp256k1ProviderImpl>();

    void SetUp() override {
      std::copy_n(go_public_key_bytes.begin(),
                  go_public_key_bytes.size(),
                  go_public_key.begin());
      std::copy_n(go_signature_bytes.begin(),
                  go_signature_bytes.size(),
                  go_signature.begin());
    }
  };

  /**
   * @given Pre-generated secp256k1 key pair, sample message and signature
   * @when Verifying pre-generated signature
   * @then Verification of the pre-generated signature must be successful
   */
  TEST_F(Secp256k1ProviderTest, PreGeneratedSignatureVerificationSuccess) {
    EXPECT_OUTCOME_TRUE(verificationResult,
                        secp256K1_no_digest_provider->verify(
                            go_message_hash, go_signature, go_public_key));
    ASSERT_TRUE(verificationResult);
  }

  /**
   * @given Sample message to sign and verify
   * @when Generating new key pair, signature and verification of this signature
   * @then Generating key pair, signature and it's verification must be
   * successful
   */
  TEST_F(Secp256k1ProviderTest, GenerateSignatureSuccess) {
    EXPECT_OUTCOME_TRUE(key_pair, secp256K1_provider->generate());
    EXPECT_OUTCOME_TRUE(
        signature,
        secp256K1_provider->sign(go_message_hash, key_pair.private_key));
    EXPECT_OUTCOME_TRUE(verificationResult,
                        secp256K1_provider->verify(
                            go_message_hash, signature, key_pair.public_key));
    ASSERT_TRUE(verificationResult);
  }

  /**
   * @given Sample message to sign and verify
   * @when Generating new signature and verifying with different public key
   * @then Signature for different public key must be invalid
   */
  TEST_F(Secp256k1ProviderTest, VerifySignatureInvalidKeyFailure) {
    EXPECT_OUTCOME_TRUE(key_pair1, secp256K1_provider->generate());
    EXPECT_OUTCOME_TRUE(key_pair2, secp256K1_provider->generate());
    EXPECT_OUTCOME_TRUE(
        signature,
        secp256K1_provider->sign(go_message_hash, key_pair1.private_key));
    EXPECT_OUTCOME_TRUE(verificationResult,
                        secp256K1_provider->verify(
                            go_message_hash, signature, key_pair2.public_key));
    ASSERT_FALSE(verificationResult);
  }

  /**
   * @given Key pair and sample message to sign
   * @when Generating and verifying invalid signature
   * @then Invalid signature verification must be unsuccessful
   */
  TEST_F(Secp256k1ProviderTest, VerifyInvalidSignaturFailure) {
    EXPECT_OUTCOME_TRUE(key_pair, secp256K1_provider->generate());
    EXPECT_OUTCOME_TRUE(
        signature,
        secp256K1_provider->sign(go_message_hash, key_pair.private_key));
    signature[0] = 0;  // Modify signature
    EXPECT_OUTCOME_TRUE(verificationResult,
                        secp256K1_provider->verify(
                            go_message_hash, signature, key_pair.public_key));
    ASSERT_FALSE(verificationResult);
  }

  /**
   * @given Sample message and wrong signature
   * @when Recover pubkey from message and signature
   * @then Recovery must be unsuccessful
   */
  TEST_F(Secp256k1ProviderTest, RecoverInvalidSignatureFailure) {
    SignatureCompact wrong_signature{};
    wrong_signature[64] = 99;
    EXPECT_OUTCOME_ERROR(
        Secp256k1Error::kSignatureParseError,
        secp256K1_provider->recoverPublicKey(go_message_hash, wrong_signature));
  }

  /**
   * @given Sample message, signature, and pubkey from go-secp256k1
   * (https://github.com/ipsn/go-secp256k1/blob/master/secp256_test.go#L206)
   * Pay attention that hash function is not used for message (it is already 32
   * byte long)
   * @when Recover pubkey from message and signature
   * @then Recovery is successful, public key returned
   */
  TEST_F(Secp256k1ProviderTest, RecoverSuccess) {
    EXPECT_OUTCOME_TRUE(public_key,
                        secp256K1_no_digest_provider->recoverPublicKey(
                            go_message_hash, go_signature));
    EXPECT_EQ(public_key, go_public_key);
  }

  /**
   * @given message and new keypair generated
   * @when sign message and then recover public key from signature
   * @then public key is the same
   */
  TEST_F(Secp256k1ProviderTest, GenerateSignRecover) {
    EXPECT_OUTCOME_TRUE(keypair, secp256K1_provider->generate());
    EXPECT_OUTCOME_TRUE(
        sig, secp256K1_provider->sign(go_message_hash, keypair.private_key));
    EXPECT_OUTCOME_TRUE(
        public_key, secp256K1_provider->recoverPublicKey(go_message_hash, sig));
    EXPECT_EQ(keypair.public_key, public_key);
  }

  /**
   * @given keypair and message
   * @when signature created twice for the same input data
   * @then result the same
   */
  TEST_F(Secp256k1ProviderTest, SignatureDetermenistic) {
    EXPECT_OUTCOME_TRUE(keypair, secp256K1_provider->generate());
    EXPECT_OUTCOME_TRUE(
        sig1, secp256K1_provider->sign(go_message_hash, keypair.private_key));
    EXPECT_OUTCOME_TRUE(
        sig2, secp256K1_provider->sign(go_message_hash, keypair.private_key));
    EXPECT_EQ(sig1, sig2);
  }

}  // namespace fc::crypto::secp256k1
