/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "crypto/bls/impl/bls_provider_impl.hpp"
#include "crypto/secp256k1/secp256k1_provider.hpp"
#include "primitives/address/address_builder.hpp"
#include "primitives/address/impl/address_builder_impl.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"
#include "vm/message/impl/message_signer_bls.hpp"
#include "vm/message/impl/message_signer_secp256k1.hpp"
#include "vm/message/message_util.hpp"

#include "common/buffer.hpp"

using fc::crypto::bls::BlsProvider;
using fc::crypto::bls::impl::BlsProviderImpl;
using BlsPrivateKey = fc::crypto::bls::PrivateKey;
using BlsPublicKey = fc::crypto::bls::PublicKey;
using fc::crypto::secp256k1::Secp256k1Provider;
using fc::crypto::secp256k1::Secp256k1ProviderImpl;
using Secp256k1PrivateKey = fc::crypto::secp256k1::PrivateKey;
using Secp256k1PublicKey = fc::crypto::secp256k1::PublicKey;
using fc::primitives::BigInt;
using fc::primitives::address::Address;
using fc::primitives::address::AddressBuilder;
using fc::primitives::address::AddressBuilderImpl;
using fc::primitives::address::Network;
using fc::vm::message::BlsMessageSigner;
using fc::vm::message::decode;
using fc::vm::message::Secp256k1MessageSigner;
using fc::vm::message::serialize;
using fc::vm::message::Signature;
using fc::vm::message::SignedMessage;
using fc::vm::message::UnsignedMessage;

using Bytes = std::vector<uint8_t>;

Address fromHex(const std::vector<uint8_t> &bytes) {
  return fc::primitives::address::decode(bytes).value();
}

struct MessageTest : public testing::Test {
  std::shared_ptr<Secp256k1Provider> secp256k1_provider;
  std::shared_ptr<BlsProvider> bls_provider;

  std::vector<Bytes> secp256k1_private_keys;
  std::vector<Bytes> bls_private_keys;

  std::shared_ptr<BlsMessageSigner> bls_message_signer;
  std::shared_ptr<Secp256k1MessageSigner> secp256k1_message_signer;

  Address some_id_address{Network::TESTNET, 1001};
  std::shared_ptr<AddressBuilder> address_builder;

  void SetUp() override {
    secp256k1_private_keys = std::move(std::vector<Bytes>{
        "a175d54a181a0a7e361abce2fd683d082734dd566c66e961485b34229cfba05a"_unhex,
        "a45bb4b78ef4b9fea140b202c6d8019c1a1395bac19a3ec9520d808d0c6ff668"_unhex,
        "b25a1854ae00534efb6a95c6b6e974d092611feedd675513b7a415c2bb41c514"_unhex,
        "2a095df1b693215e1cae92715bea66a8f9c66b137162eabf7f5aaa6e900b3069"_unhex,
        "f9879433e6c8363331c39da83284cc7c166512038b7658f2ff0dad6f1091bb16"_unhex,
        "f37cb8bdd4cc899896bc657d7ca6a4ab6a42c48b017f97820bb431eedec1f264"_unhex,
        "5cd061233750404588f560c6e72a1395cae0d6d82034bcc1face2ed2b0ec625a"_unhex,
        "cfa3492ca3db43a51ec271d785bce7ba765ddf83c344284c68374f17f79c3686"_unhex,
        "0864f17e918f37e242b798a66f00678ab2997f75f25894aca77a5d7c0cee25b0"_unhex,
        "16a0d0f2481eec942330a547ea0d85b1ce8c3915f14d5584671d563f5bbd455c"_unhex,
        "e2aa65d9f1a31414bc7b00c31e15cd9f44256414dd88c9d96c43f62051f4830f"_unhex,
        "f89e9c64741245c38efb5a977609f54489edfad4a0f2fd9d19cde3ad4992a02a"_unhex,
        "e8df450cc9601707db12f2c93e2b8c5a96982a5ee0f38a815194c24ce2118a9f"_unhex,
        "d4fc96daf24a99ecad718a586651e81d6844746a4c07305e124479c4ed49bd23"_unhex,
        "9a4ae45079798b2f8dd7c043171e8a53f5c4cea3ae85f45e550153a16e4f4d9f"_unhex});

    bls_private_keys = std::move(std::vector<Bytes>{
        "bc9dc79fec596af2b50fa3be8e08b09c004a77ca1f963a078c80cbea0f6cf156"_unhex,
        "b04074ca6557957b271961380007accfb0bed3db1c802663c03032f464e0e235"_unhex,
        "da3881f11606de86a2bba95bb04d580911c42b71316c1c0202031e96b2446426"_unhex,
        "7a973cb0d1ed378097b19596967c3797426f77f7fba0e2f8aede53f6713b1905"_unhex,
        "719f408735d359cfc3ce53f9b4fb4198f8202e114094ae05c327547b976d0a24"_unhex,
        "91df89658ec2ea51d8036c6901ae59271c91e7da153ee802445e646f1aaea758"_unhex,
        "88f79f30e510e9fea27e63a4a12bad882288eb89c837f3de65e6135c690a6d4a"_unhex,
        "1e638c63b7034b8af7b29ea2d1e76520ac877f87dd80d445c4dff89422d40f33"_unhex,
        "d64c231b69fd0cff5a24f272cd4dd6461727bca232c42edf06d7ac25c4623e04"_unhex,
        "353f6de9f869878f58e632595a5ba1eb5758857930c46ccd0162616f8000d00d"_unhex,
        "ac2881970a9360849b14420de3af3af69362752f8b25950fe695e01473f3c33a"_unhex,
        "216bfc8aa24251812aebcc87b1318d354f80125d66950ee7ca38a8aaedad891f"_unhex,
        "7680a2a481c8d518f0030fb732b93db25c3929959f49442832975aac8a76b31d"_unhex,
        "f25c470ff249ea90e4f9a778e0f83dc26d4f2e5b8e2d9aad41287611290b4228"_unhex,
        "606474f0bfbc1b848a5bb335a3a890f71c72423c2fa1e70c8f61063cd989892e"_unhex});

    bls_provider = std::make_shared<BlsProviderImpl>();
    secp256k1_provider = std::make_shared<Secp256k1ProviderImpl>();

    bls_message_signer = std::make_shared<BlsMessageSigner>(bls_provider);
    secp256k1_message_signer =
        std::make_shared<Secp256k1MessageSigner>(secp256k1_provider);

    address_builder = std::make_shared<AddressBuilderImpl>();
  }
};

/**
 * @given A set of UnsignedMessages from BlsPublicKey or Secp256k1PublicKey
 * addresses to a dummy ID address
 * @when Serializing them to cbor and then decoding them back
 * @then Return messages match the original ones
 */
TEST_F(MessageTest, UnsignedMessagesEncodingRoundTrip) {
  uint64_t nonce = 1234;
  for (auto it = this->bls_private_keys.begin();
       it != this->bls_private_keys.end();
       it++) {
    BlsPrivateKey private_key{};
    std::copy_n(it->begin(), private_key.size(), private_key.begin());
    EXPECT_OUTCOME_TRUE(public_key,
                        this->bls_provider->derivePublicKey(private_key));
    EXPECT_OUTCOME_TRUE(from,
                        this->address_builder->makeFromBlsPublicKey(
                            Network::TESTNET, public_key));
    UnsignedMessage msg{
        from,                      // from Address
        some_id_address,           // to Address
        nonce++,                   // nonce
        BigInt(0x01),              // transfer value
        1u,                        // method num
        "0123ceed9876feed"_unhex,  // method params
        BigInt(0x100),             // gasPrice
        BigInt(0x1000000)          // gasLimit
    };
    Bytes encoded = serialize<UnsignedMessage>(msg);
    EXPECT_OUTCOME_TRUE(decoded, decode<UnsignedMessage>(encoded));
    EXPECT_EQ(msg, decoded);
  }
  for (auto it = this->secp256k1_private_keys.begin();
       it != this->secp256k1_private_keys.end();
       it++) {
    Secp256k1PrivateKey private_key{};
    std::copy_n(it->begin(), private_key.size(), private_key.begin());
    EXPECT_OUTCOME_TRUE(public_key,
                        this->secp256k1_provider->derivePublicKey(private_key));
    EXPECT_OUTCOME_TRUE(from,
                        this->address_builder->makeFromSecp256k1PublicKey(
                            Network::TESTNET, public_key));
    UnsignedMessage msg{from,
                        some_id_address,
                        nonce++,
                        BigInt(0xffff),
                        2u,
                        ""_unhex,
                        BigInt(0x100),
                        BigInt(0x10000)};
    Bytes encoded = serialize<UnsignedMessage>(msg);
    // std::cout << "encoded: " << fc::common::Buffer(encoded) << std::endl;
    EXPECT_OUTCOME_TRUE(decoded, decode<UnsignedMessage>(encoded));
    EXPECT_EQ(msg, decoded);
  }
}

/**
 * @given A set of UnsignedMessages from BlsPublicKey or Secp256k1PublicKey
 * addresses to a dummy ID address
 * @when Signing the messages with respecitve private keys and then verifying
 * the signatures with corresponding public keys
 * @then Verification returns 'true'
 */
TEST_F(MessageTest, VerifySignedMessagesSuccess) {
  uint64_t nonce = 1234;
  for (auto it = this->bls_private_keys.begin();
       it != this->bls_private_keys.end();
       it++) {
    BlsPrivateKey private_key{};
    std::copy_n(it->begin(), private_key.size(), private_key.begin());
    EXPECT_OUTCOME_TRUE(public_key,
                        this->bls_provider->derivePublicKey(private_key));
    EXPECT_OUTCOME_TRUE(
        from,
        address_builder->makeFromBlsPublicKey(Network::TESTNET, public_key));
    UnsignedMessage msg{from,
                        some_id_address,
                        nonce++,
                        BigInt(0x01),
                        1u,
                        "0123ceed9876feed"_unhex,
                        BigInt(0x100),
                        BigInt(0x1000000)};
    EXPECT_OUTCOME_TRUE(signed_message,
                        this->bls_message_signer->sign(msg, private_key));
    EXPECT_OUTCOME_TRUE(
        ok, this->bls_message_signer->verify(signed_message, public_key));
    EXPECT_TRUE(ok);
  }
  for (auto it = this->secp256k1_private_keys.begin();
       it != this->secp256k1_private_keys.end();
       it++) {
    Secp256k1PrivateKey private_key{};
    std::copy_n(it->begin(), private_key.size(), private_key.begin());
    EXPECT_OUTCOME_TRUE(public_key,
                        this->secp256k1_provider->derivePublicKey(private_key));
    EXPECT_OUTCOME_TRUE(from,
                        address_builder->makeFromSecp256k1PublicKey(
                            Network::TESTNET, public_key));
    UnsignedMessage msg{from,
                        some_id_address,
                        nonce++,
                        BigInt(0xffff),
                        2u,
                        ""_unhex,
                        BigInt(0x100),
                        BigInt(0x10000)};
    EXPECT_OUTCOME_TRUE(signed_message,
                        this->secp256k1_message_signer->sign(msg, private_key));
    EXPECT_OUTCOME_TRUE(
        ok, this->secp256k1_message_signer->verify(signed_message, public_key));
    EXPECT_TRUE(ok);
  }
}

/**
 * @given A set of UnsignedMessages from BlsPublicKey or Secp256k1PublicKey
 * addresses to a dummy ID address
 * @when Signing the messages with a private keys and then verifying the
 * signatures with a public keys from a different keypair
 * @then Verification returns 'false'
 */
TEST_F(MessageTest, VerifySignedMessagesFailure) {
  uint64_t nonce = 1234;
  for (auto it = std::next(this->bls_private_keys.begin());
       it != this->bls_private_keys.end();
       it++) {
    BlsPrivateKey private_key{}, signing_key{};
    std::copy_n(it->begin(), private_key.size(), private_key.begin());
    std::copy_n(
        std::prev(it)->begin(), signing_key.size(), signing_key.begin());
    EXPECT_OUTCOME_TRUE(public_key,
                        this->bls_provider->derivePublicKey(private_key));
    EXPECT_OUTCOME_TRUE(
        from,
        address_builder->makeFromBlsPublicKey(Network::TESTNET, public_key));
    UnsignedMessage msg{from,
                        some_id_address,
                        nonce++,
                        BigInt(0x01),
                        1u,
                        "0123ceed9876feed"_unhex,
                        BigInt(0x100),
                        BigInt(0x1000000)};
    EXPECT_OUTCOME_TRUE(signed_message,
                        this->bls_message_signer->sign(msg, signing_key));
    EXPECT_OUTCOME_TRUE(
        not_ok, this->bls_message_signer->verify(signed_message, public_key));
    EXPECT_FALSE(not_ok);
  }
  for (auto it = std::next(this->secp256k1_private_keys.begin());
       it != this->secp256k1_private_keys.end();
       it++) {
    Secp256k1PrivateKey private_key{}, signing_key{};
    std::copy_n(it->begin(), private_key.size(), private_key.begin());
    std::copy_n(
        std::prev(it)->begin(), signing_key.size(), signing_key.begin());
    EXPECT_OUTCOME_TRUE(public_key,
                        this->secp256k1_provider->derivePublicKey(private_key));
    EXPECT_OUTCOME_TRUE(from,
                        address_builder->makeFromSecp256k1PublicKey(
                            Network::TESTNET, public_key));
    UnsignedMessage msg{from,
                        some_id_address,
                        nonce++,
                        BigInt(0xffff),
                        2u,
                        ""_unhex,
                        BigInt(0x100),
                        BigInt(0x10000)};
    EXPECT_OUTCOME_TRUE(signed_message,
                        this->secp256k1_message_signer->sign(msg, signing_key));
    EXPECT_OUTCOME_TRUE(
        not_ok,
        this->secp256k1_message_signer->verify(signed_message, public_key));
    EXPECT_FALSE(not_ok);
  }
}

/**
 * @given A set of SignedMessages from BlsPublicKey or Secp256k1PublicKey
 * addresses to a dummy ID address
 * @when Serializing them to cbor and then decoding them back
 * @then Return messages match the original ones
 */
TEST_F(MessageTest, SignedMessagesEncodingRoundTrip) {
  uint64_t nonce = 1234;
  for (auto it = this->bls_private_keys.begin();
       it != this->bls_private_keys.end();
       it++) {
    BlsPrivateKey private_key{};
    std::copy_n(it->begin(), private_key.size(), private_key.begin());
    EXPECT_OUTCOME_TRUE(public_key,
                        this->bls_provider->derivePublicKey(private_key));
    EXPECT_OUTCOME_TRUE(
        from,
        address_builder->makeFromBlsPublicKey(Network::TESTNET, public_key));
    UnsignedMessage msg{from,
                        some_id_address,
                        nonce++,
                        BigInt(0x01),
                        1u,
                        "0123ceed9876feed"_unhex,
                        BigInt(0x100),
                        BigInt(0x1000000)};
    EXPECT_OUTCOME_TRUE(signed_message,
                        this->bls_message_signer->sign(msg, private_key));
    Bytes encoded = serialize<SignedMessage>(signed_message);
    EXPECT_OUTCOME_TRUE(decoded, decode<SignedMessage>(encoded));
    EXPECT_TRUE(signed_message.message == decoded.message
                && signed_message.signature.type == decoded.signature.type
                && signed_message.signature.data == decoded.signature.data);
  }
  for (auto it = this->secp256k1_private_keys.begin();
       it != this->secp256k1_private_keys.end();
       it++) {
    Secp256k1PrivateKey private_key{};
    std::copy_n(it->begin(), private_key.size(), private_key.begin());
    EXPECT_OUTCOME_TRUE(public_key,
                        this->secp256k1_provider->derivePublicKey(private_key));
    EXPECT_OUTCOME_TRUE(from,
                        address_builder->makeFromSecp256k1PublicKey(
                            Network::TESTNET, public_key));
    UnsignedMessage msg{from,
                        some_id_address,
                        nonce++,
                        BigInt(0xffff),
                        2u,
                        ""_unhex,
                        BigInt(0x100),
                        BigInt(0x10000)};
    EXPECT_OUTCOME_TRUE(signed_message,
                        this->secp256k1_message_signer->sign(msg, private_key));
    Bytes encoded = serialize<SignedMessage>(signed_message);
    EXPECT_OUTCOME_TRUE(decoded, decode<SignedMessage>(encoded));
    EXPECT_TRUE(signed_message.message == decoded.message
                && signed_message.signature.type == decoded.signature.type
                && signed_message.signature.data == decoded.signature.data);
  }
}