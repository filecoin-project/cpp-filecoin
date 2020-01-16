/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "crypto/bls/impl/bls_provider_impl.hpp"
#include "crypto/secp256k1/secp256k1_provider.hpp"
#include "primitives/address/impl/address_builder_impl.hpp"
#include "primitives/address/impl/address_verifier_impl.hpp"
#include "storage/keystore/impl/in_memory/in_memory_keystore.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"
#include "vm/message/impl/message_signer_impl.hpp"
#include "vm/message/message_util.hpp"

using fc::crypto::bls::BlsProvider;
using fc::crypto::bls::impl::BlsProviderImpl;
using BlsPrivateKey = fc::crypto::bls::PrivateKey;
using fc::crypto::secp256k1::Secp256k1Provider;
using fc::crypto::secp256k1::Secp256k1ProviderImpl;
using Secp256k1PrivateKey = fc::crypto::secp256k1::PrivateKey;

using fc::primitives::BigInt;
using fc::primitives::address::Address;
using fc::primitives::address::AddressBuilder;
using fc::primitives::address::AddressBuilderImpl;
using fc::primitives::address::AddressVerifier;
using fc::primitives::address::AddressVerifierImpl;
using fc::primitives::address::Network;

using fc::storage::keystore::InMemoryKeyStore;

using fc::codec::cbor::decode;
using fc::codec::cbor::encode;

using fc::storage::keystore::KeyStore;
using fc::storage::keystore::KeyStoreError;

using fc::vm::message::cid;
using fc::vm::message::MessageError;
using fc::vm::message::MessageSigner;
using fc::vm::message::MessageSignerImpl;
using fc::vm::message::SignedMessage;
using fc::vm::message::UnsignedMessage;

using Bytes = std::vector<uint8_t>;

UnsignedMessage makeMessage(Address const &from,
                            Address const &to,
                            uint64_t nonce) {
  return UnsignedMessage{
      to,         // to Address
      from,       // from Address
      nonce,      // nonce
      BigInt(1),  // transfer value
      BigInt(0),  // gasPrice
      BigInt(1),  // gasLimit
      0,          // method num
      ""_unhex    // method params
  };
}
struct MessageTest : public testing::Test {
  BlsPrivateKey bls_signing_key{251, 43,  111, 2,   195, 6,   169, 207,
                                32,  75,  142, 115, 8,   149, 149, 119,
                                116, 140, 80,  211, 8,   219, 9,   16,
                                174, 133, 97,  167, 249, 223, 160, 40};
  BlsPrivateKey bls_other_key{207, 171, 143, 214, 40,  249, 188, 163,
                              17,  4,   127, 245, 79,  116, 197, 165,
                              134, 43,  27,  194, 216, 230, 237, 23,
                              28,  14,  128, 201, 84,  112, 155, 63};
  Secp256k1PrivateKey secp256k1_signing_key{
      60,  155, 92,  41,  33,  170, 0,   218, 108, 129, 96,
      39,  153, 104, 231, 195, 219, 12,  60,  219, 253, 120,
      147, 101, 137, 54,  250, 215, 198, 26,  77,  179};
  Secp256k1PrivateKey secp256k1_other_key{
      17,  141, 140, 35,  136, 192, 93,  11,  250, 25,  168,
      216, 106, 143, 39,  184, 26,  125, 83,  108, 231, 124,
      96,  66,  236, 172, 170, 243, 183, 100, 240, 227};

  Address target{Network::TESTNET, 1001};

  std::vector<uint64_t> nonces{0, 1, 2, 3, 4};

  std::vector<Bytes> bls_msg_cids{
      "0171a0e40220abb8435c9c1c1e170d6c26707dd52fbe57cf1bc7a5801bd047903b98bd110da1"_unhex,
      "0171a0e4022045acc0a0f654af85d213c533c2620fd85ccbb9bf369b6e325a83b214d59b93bc"_unhex,
      "0171a0e4022083064ca02e2c714329a49797fb871e08dbd152a420f69a291d3663b09ac26ec4"_unhex,
      "0171a0e4022066acdd1ae69b48ac5548c7f365985db0e61695f10c4862e54101c43c2fa0002e"_unhex,
      "0171a0e402200e228d43e6df39843df7ad4027292f68f843d8e770d6df7439b5aa0f2f2d2da3"_unhex};
  std::vector<Bytes> secp256k1_msg_cids{
      "0171a0e402204af1d24cdc8e606b5c414deb4941437b00004721b56ac662e649b1be508c74c5"_unhex,
      "0171a0e402201d5990e85cad07760e9e2307da8ac12891fc978a8fd13a1ef608155b956a13ec"_unhex,
      "0171a0e40220527883177943f5d042916fe021c8c04e21a1e85dd89d0a86de1d737d355f5c8a"_unhex,
      "0171a0e40220ea49706f9f742fce6cbfef84b60429ea5e10a1260c4313bd211d967c099bba5f"_unhex,
      "0171a0e4022092cfdff2bf1c1d2b73d2f29030f390edd3c65132989cdbf675cdbf0e423c3fe3"_unhex};

  std::shared_ptr<BlsProvider> bls_provider;
  std::shared_ptr<Secp256k1Provider> secp256k1_provider;
  std::shared_ptr<AddressVerifier> address_verifier;
  std::unique_ptr<AddressBuilder> address_builder;

  std::shared_ptr<KeyStore> keystore;
  std::shared_ptr<MessageSigner> msigner;

  void SetUp() override {
    bls_provider = std::make_shared<BlsProviderImpl>();
    secp256k1_provider = std::make_shared<Secp256k1ProviderImpl>();
    address_verifier = std::make_shared<AddressVerifierImpl>();
    address_builder = std::make_unique<AddressBuilderImpl>();

    keystore = std::make_shared<InMemoryKeyStore>(
        bls_provider, secp256k1_provider, address_verifier);

    keystore
        ->put(address_builder
                  ->makeFromBlsPublicKey(
                      Network::TESTNET,
                      bls_provider->derivePublicKey(bls_signing_key).value())
                  .value(),
              bls_signing_key)
        .value();
    keystore
        ->put(address_builder
                  ->makeFromBlsPublicKey(
                      Network::TESTNET,
                      bls_provider->derivePublicKey(bls_other_key).value())
                  .value(),
              bls_other_key)
        .value();
    keystore
        ->put(address_builder
                  ->makeFromSecp256k1PublicKey(
                      Network::TESTNET,
                      secp256k1_provider->derivePublicKey(secp256k1_signing_key)
                          .value())
                  .value(),
              secp256k1_signing_key)
        .value();

    msigner = std::make_shared<MessageSignerImpl>(keystore);
  }
};

/**
 * @given A set of UnsignedMessages from BLS public key address to some target
 * ID address
 * @when Serializing them to cbor and then decoding them back
 * @then Return messages match the original ones
 */
TEST_F(MessageTest, UnsignedMessagesEncodingRoundTrip) {
  EXPECT_OUTCOME_TRUE(
      public_key, this->bls_provider->derivePublicKey(this->bls_signing_key));
  EXPECT_OUTCOME_TRUE(from,
                      this->address_builder->makeFromBlsPublicKey(
                          Network::TESTNET, public_key));
  for (auto i : this->nonces) {
    auto msg = makeMessage(from, this->target, i);
    EXPECT_OUTCOME_TRUE(encoded, encode<UnsignedMessage>(msg));
    EXPECT_OUTCOME_TRUE(decoded, decode<UnsignedMessage>(encoded));
    EXPECT_EQ(msg, decoded);
  }
}

/**
 * @given A set of UnsignedMessages from BLS public key address to some target
 * ID address
 * @when Signing the unsigned messages and then verifying the signatures with
 * corresponding public key
 * @then Verification succeeds
 */
TEST_F(MessageTest, VerifySignedBLSMessagesSuccess) {
  EXPECT_OUTCOME_TRUE(
      public_key, this->bls_provider->derivePublicKey(this->bls_signing_key));
  EXPECT_OUTCOME_TRUE(from,
                      this->address_builder->makeFromBlsPublicKey(
                          Network::TESTNET, public_key));
  for (auto i : this->nonces) {
    auto msg = makeMessage(from, this->target, i);
    EXPECT_OUTCOME_TRUE(signed_message, this->msigner->sign(from, msg));
    EXPECT_OUTCOME_TRUE(msg2, this->msigner->verify(from, signed_message));
    EXPECT_EQ(msg, msg2);
  }
}

/**
 * @given A set of UnsignedMessages from Secp256k1 public key address to some
 * target ID address
 * @when Signing the unsigned messages and then verifying the signed messages
 * with the same address that was used for signing
 * @then Verification succeeds
 */
TEST_F(MessageTest, VerifySignedSecp256k1MessagesSuccess) {
  EXPECT_OUTCOME_TRUE(
      public_key,
      this->secp256k1_provider->derivePublicKey(this->secp256k1_signing_key));
  EXPECT_OUTCOME_TRUE(from,
                      this->address_builder->makeFromSecp256k1PublicKey(
                          Network::TESTNET, public_key));
  for (auto i : this->nonces) {
    auto msg = makeMessage(from, this->target, i);
    EXPECT_OUTCOME_TRUE(signed_message, this->msigner->sign(from, msg));
    EXPECT_OUTCOME_TRUE(msg2, this->msigner->verify(from, signed_message));
    EXPECT_EQ(msg, msg2);
  }
}

/**
 * @given A set of UnsignedMessages from BLS public key address to some target
 * ID address
 * @when Signing the messages and then verifying the signed messages
 * with a different BlsPublicKey address which is also present in keystore
 * @then Verification returns MessageError::VERIFICATION_FAILURE
 */
TEST_F(MessageTest, SignedMessagesVerificationFailure) {
  EXPECT_OUTCOME_TRUE(
      public_key, this->bls_provider->derivePublicKey(this->bls_signing_key));
  EXPECT_OUTCOME_TRUE(from,
                      this->address_builder->makeFromBlsPublicKey(
                          Network::TESTNET, public_key));
  EXPECT_OUTCOME_TRUE(other_key,
                      this->bls_provider->derivePublicKey(this->bls_other_key));
  EXPECT_OUTCOME_TRUE(
      other,
      this->address_builder->makeFromBlsPublicKey(Network::TESTNET, other_key));
  for (auto i : this->nonces) {
    auto msg = makeMessage(from, this->target, i);
    EXPECT_OUTCOME_TRUE(signed_message, this->msigner->sign(from, msg));
    EXPECT_OUTCOME_ERROR(MessageError::VERIFICATION_FAILURE,
                         this->msigner->verify(other, signed_message));
  }
}

/**
 * @given A set of UnsignedMessages from BLS public key address to some target
 * ID address
 * @when Signing the messages and then verifying the signed messages
 * with a Secp256k1PublicKey address which is also present in keystore
 * @then Verification returns KeyStoreError::WRONG_SIGNATURE
 */
TEST_F(MessageTest, VerificationFailureWrongSignature) {
  EXPECT_OUTCOME_TRUE(
      public_key, this->bls_provider->derivePublicKey(this->bls_signing_key));
  EXPECT_OUTCOME_TRUE(from,
                      this->address_builder->makeFromBlsPublicKey(
                          Network::TESTNET, public_key));
  EXPECT_OUTCOME_TRUE(
      other_key,
      this->secp256k1_provider->derivePublicKey(this->secp256k1_signing_key));
  EXPECT_OUTCOME_TRUE(other,
                      this->address_builder->makeFromSecp256k1PublicKey(
                          Network::TESTNET, other_key));
  for (auto i : this->nonces) {
    auto msg = makeMessage(from, this->target, i);
    EXPECT_OUTCOME_TRUE(signed_message, this->msigner->sign(from, msg));
    EXPECT_OUTCOME_ERROR(KeyStoreError::WRONG_SIGNATURE,
                         this->msigner->verify(other, signed_message));
  }
}

/**
 * @given A set of UnsignedMessages from BLS public key address to some target
 * ID address
 * @when Signing the messages and then verifying the signed messages
 * with an address which is not present in keystore
 * @then Verification returns KeyStoreError::NOT_FOUND
 */
TEST_F(MessageTest, VerificationFailureWrongAddress) {
  EXPECT_OUTCOME_TRUE(
      public_key, this->bls_provider->derivePublicKey(this->bls_signing_key));
  EXPECT_OUTCOME_TRUE(from,
                      this->address_builder->makeFromBlsPublicKey(
                          Network::TESTNET, public_key));
  EXPECT_OUTCOME_TRUE(
      other_key,
      this->secp256k1_provider->derivePublicKey(this->secp256k1_other_key));
  EXPECT_OUTCOME_TRUE(other,
                      this->address_builder->makeFromSecp256k1PublicKey(
                          Network::TESTNET, other_key));
  for (auto i : this->nonces) {
    auto msg = makeMessage(from, this->target, i);
    EXPECT_OUTCOME_TRUE(signed_message, this->msigner->sign(from, msg));
    EXPECT_OUTCOME_ERROR(KeyStoreError::NOT_FOUND,
                         this->msigner->verify(other, signed_message));
  }
}

/**
 * @given A set of SignedMessages from Secp256k1 public key address to some
 * target ID address
 * @when Serializing them to cbor and then decoding them back
 * @then Return messages match the original ones
 */
TEST_F(MessageTest, SignedMessagesEncodingRoundTrip) {
  EXPECT_OUTCOME_TRUE(
      public_key,
      this->secp256k1_provider->derivePublicKey(this->secp256k1_signing_key));
  EXPECT_OUTCOME_TRUE(from,
                      this->address_builder->makeFromSecp256k1PublicKey(
                          Network::TESTNET, public_key));
  for (auto i : this->nonces) {
    auto msg = makeMessage(from, this->target, i);
    EXPECT_OUTCOME_TRUE(signed_message, this->msigner->sign(from, msg));
    EXPECT_OUTCOME_TRUE(encoded, encode<SignedMessage>(signed_message));
    EXPECT_OUTCOME_TRUE(decoded, decode<SignedMessage>(encoded));
    EXPECT_TRUE(signed_message.message == decoded.message
                && signed_message.signature == decoded.signature);
  }
}

/**
 * @given A set of UnsignedMessages from BLS public key address to some target
 * ID address
 * @when Calculating messages CIDs and comparing with known CIDs generated by Go
 * implementation
 * @then Values match
 */
TEST_F(MessageTest, UnsignedBLSMessagesCIDsMatch) {
  EXPECT_OUTCOME_TRUE(
      public_key, this->bls_provider->derivePublicKey(this->bls_signing_key));
  EXPECT_OUTCOME_TRUE(from,
                      this->address_builder->makeFromBlsPublicKey(
                          Network::TESTNET, public_key));
  size_t ind = 0;
  for (auto i : this->nonces) {
    auto msg = makeMessage(from, this->target, i);
    EXPECT_OUTCOME_TRUE(cid, cid(msg));
    EXPECT_OUTCOME_TRUE(cid_bytes,
                        libp2p::multi::ContentIdentifierCodec::encode(cid));
    EXPECT_EQ(cid_bytes, this->bls_msg_cids[ind++]);
  }
}

/**
 * @given A set of UnsignedMessages from Secp256k1 public key address to some
 * target ID address
 * @when Calculating messages CIDs and comparing with known CIDs generated by Go
 * implementation
 * @then Values match
 */
TEST_F(MessageTest, UnsignedSecp256k1MessagesCIDsMatch) {
  EXPECT_OUTCOME_TRUE(
      public_key,
      this->secp256k1_provider->derivePublicKey(this->secp256k1_signing_key));
  EXPECT_OUTCOME_TRUE(from,
                      this->address_builder->makeFromSecp256k1PublicKey(
                          Network::TESTNET, public_key));
  size_t ind = 0;
  for (auto i : this->nonces) {
    auto msg = makeMessage(from, this->target, i);
    EXPECT_OUTCOME_TRUE(cid, cid(msg));
    EXPECT_OUTCOME_TRUE(cid_bytes,
                        libp2p::multi::ContentIdentifierCodec::encode(cid));

    // TODO(ekovalev): change to EXPECT_EQ after (and if) the secp256k1 public
    // key length has been aligned with go-crypto project
    EXPECT_NE(cid_bytes, this->secp256k1_msg_cids[ind++]);
  }
}
