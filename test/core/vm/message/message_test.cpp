/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include <map>

#include "crypto/bls/impl/bls_provider_impl.hpp"
#include "crypto/secp256k1/secp256k1_provider.hpp"
#include "primitives/address/impl/address_builder_impl.hpp"
#include "primitives/address/impl/address_verifier_impl.hpp"
#include "storage/keystore/impl/in_memory/in_memory_keystore.hpp"
#include "testutil/cbor.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"
#include "vm/message/impl/message_signer_impl.hpp"
#include "vm/message/message_util.hpp"

using fc::crypto::bls::BlsProvider;
using fc::crypto::bls::BlsProviderImpl;
using BlsPrivateKey = fc::crypto::bls::PrivateKey;
using BlsPublicKey = fc::crypto::bls::PublicKey;
using fc::crypto::secp256k1::Secp256k1Provider;
using fc::crypto::secp256k1::Secp256k1ProviderImpl;
using Secp256k1PrivateKey = fc::crypto::secp256k1::PrivateKey;
using Secp256k1PublicKey = fc::crypto::secp256k1::PublicKey;
using fc::crypto::signature::Signature;

using fc::primitives::BigInt;
using fc::primitives::address::Address;
using fc::primitives::address::AddressBuilder;
using fc::primitives::address::AddressBuilderImpl;
using fc::primitives::address::AddressVerifier;
using fc::primitives::address::AddressVerifierImpl;
using fc::primitives::address::Network;

using fc::storage::keystore::InMemoryKeyStore;

using fc::storage::keystore::KeyStore;
using fc::storage::keystore::KeyStoreError;

using fc::vm::message::cid;
using fc::vm::message::MessageError;
using fc::vm::message::MessageSigner;
using fc::vm::message::MessageSignerImpl;
using fc::vm::message::MethodNumber;
using fc::vm::message::MethodParams;
using fc::vm::message::SignedMessage;
using fc::vm::message::UnsignedMessage;

using fc::visit_in_place;

using Bytes = std::vector<uint8_t>;

using CryptoProvider = boost::variant<std::shared_ptr<BlsProvider>,
                                      std::shared_ptr<Secp256k1Provider>>;

UnsignedMessage makeMessage(Address const &from,
                            Address const &to,
                            uint64_t nonce) {
  return UnsignedMessage{
      to,                     // to Address
      from,                   // from Address
      nonce,                  // nonce
      BigInt(1),              // transfer value
      BigInt(0),              // gasPrice
      BigInt(1),              // gasLimit
      MethodNumber{0},        // method num
      MethodParams{""_unhex}  // method params
  };
}

Address addKeyGetAddress(
    const std::array<uint8_t, 32> &private_key,
    const CryptoProvider &provider,
    const std::shared_ptr<KeyStore> &keystore,
    const std::shared_ptr<AddressBuilder> &address_builder) {
  return visit_in_place(
      provider,
      [&](const std::shared_ptr<BlsProvider> &p) {
        auto address =
            address_builder
                ->makeFromBlsPublicKey(Network::TESTNET,
                                       p->derivePublicKey(private_key).value())
                .value();
        keystore->put(address, private_key).value();
        return address;
      },
      [&](const std::shared_ptr<Secp256k1Provider> &p) {
        auto address =
            address_builder
                ->makeFromSecp256k1PublicKey(
                    Network::TESTNET, p->derive(private_key).value())
                .value();
        keystore->put(address, private_key).value();
        return address;
      });
}

struct MessageTest : public testing::Test {
  UnsignedMessage message;

  Address bls, secp;

  std::shared_ptr<BlsProvider> bls_provider;
  std::shared_ptr<Secp256k1Provider> secp256k1_provider;
  std::shared_ptr<AddressVerifier> address_verifier;
  std::shared_ptr<AddressBuilder> address_builder;

  std::shared_ptr<KeyStore> keystore;
  std::shared_ptr<MessageSigner> msigner;

  void SetUp() override {
    bls_provider = std::make_shared<BlsProviderImpl>();
    secp256k1_provider = std::make_shared<Secp256k1ProviderImpl>();
    address_verifier = std::make_shared<AddressVerifierImpl>();
    address_builder = std::make_shared<AddressBuilderImpl>();

    keystore = std::make_shared<InMemoryKeyStore>(
        bls_provider, secp256k1_provider, address_verifier);

    bls = addKeyGetAddress(
        "8e8c5263df0022d8e29cab943d57d851722c38ee1dbe7f8c29c0498156496f29"_blob32,
        bls_provider,
        keystore,
        address_builder);
    secp = addKeyGetAddress(
        "7008136b505aa01e406f72204668865852186756c95cd3a7e5184ef7b8f62058"_blob32,
        secp256k1_provider,
        keystore,
        address_builder);

    msigner = std::make_shared<MessageSignerImpl>(keystore);

    message = makeMessage(bls, Address{Network::TESTNET, 1001}, 0);
  }
};

/**
 * @given An UnsignedMessage and having it signed with BLS address
 * @when Comparing the the signed message CID with the pre-computed value from
 * the reference Go implementation
 * @then Values match
 */
TEST_F(MessageTest, BlsSignedMessageCID) {
  EXPECT_OUTCOME_TRUE(signed_message, msigner->sign(bls, message));
  EXPECT_OUTCOME_EQ(
      cid(signed_message),
      "0171a0e40220c4534185b0af5fdc31e79efac6fd64e514aad37a6569e69f09d35ba221e4007e"_cid);
}

// TODO(ekovalev): the following test is currently disabled due to Secp256k1
// signature non-determinism. Yet the reference Go implementation has consistent
// Secp256k1 signatures. More research required
/**
 * @given An UnsignedMessage and having it signed with Secp256k1 address
 * @when Comparing the the signed message CID with the pre-computed value from
 * the reference Go implementation
 * @then Values match
 */
TEST_F(MessageTest, DISABLED_Secp256k1SignedMessageCID) {
  EXPECT_OUTCOME_TRUE(signed_message, msigner->sign(secp, message));
  EXPECT_OUTCOME_EQ(
      cid(signed_message),
      "0171a0e402201c9a054f1d0918cf9e215903078d5fa72e3d4de95b11ba5c49c1dffaf1d917c2"_cid);
}

/**
 * @given An UnsignedMessage
 * @when Serializing it to CBOR and comparing with pre-computed value; then
 * decoding the UnsignedMessage back and re-encoding it to CBOR again to ensure
 * consistency
 * @then All values match
 */
TEST_F(MessageTest, UnsignedMessagesEncoding) {
  expectEncodeAndReencode<UnsignedMessage>(
      message,
      "884300e907583103b70dcae7107be6aeb609fd0951d38983d8137192d03ded4754204726817485360026814114f72e66d05155d897cfe72700420001404200010040"_unhex);
}

/**
 * @given An UnsignedMessage and having it signed on behalf of any address
 * @when Serializing the signed message to CBOR and comparing with pre-computed
 * value; then decoding the UnsignedMessage back and re-encoding it to CBOR
 * again
 * @then All values match
 */
TEST_F(MessageTest, SignedMessagesEncoding) {
  EXPECT_OUTCOME_TRUE(signed_message, msigner->sign(bls, message));
  expectEncodeAndReencode<SignedMessage>(
      signed_message,
      "82884300e907583103b70dcae7107be6aeb609fd0951d38983d8137192d03ded4754204726817485360026814114f72e66d05155d897cfe727004200014042000100405861028ce1601b1324a9301685d5a0e5fd9611476f2b3d358fccc5798a9f173c0f2c90f0a4bc329bf6cdc963d33767ead3e9580ab40e3b068813e8388753d32846d1b539e1f8aefa2a2dcf1672f3841ccf1ebc861a4e240e6318dde4aaadb0023ff25f"_unhex);
}

/**
 * @given An UnsignedMessage and having it signed with BLS address
 * @when Serializing the Signature to CBOR and comparing with pre-computed
 * value; then decoding the Signature back and re-encoding it to CBOR again
 * @then All values match
 */
TEST_F(MessageTest, BlsSignatureEncoding) {
  EXPECT_OUTCOME_TRUE(signed_message, msigner->sign(bls, message));
  expectEncodeAndReencode<Signature>(
      signed_message.signature,
      "5861028ce1601b1324a9301685d5a0e5fd9611476f2b3d358fccc5798a9f173c0f2c90f0a4bc329bf6cdc963d33767ead3e9580ab40e3b068813e8388753d32846d1b539e1f8aefa2a2dcf1672f3841ccf1ebc861a4e240e6318dde4aaadb0023ff25f"_unhex);
}

// TODO(ekovalev): the following test is currently disabled due to Secp256k1
// signature non-determinism. Yet the reference Go implementation has consistent
// Secp256k1 signatures. More research required
/**
 * @given An UnsignedMessage and having it signed with Secp256k1 address
 * @when Serializing the Signature to CBOR and comparing with pre-computed
 * value; then decoding the Signature back and re-encoding it to CBOR again
 * @then All values match
 */

TEST_F(MessageTest, DISABLED_Secp256k1SignatureEncoding) {
  EXPECT_OUTCOME_TRUE(signed_message, msigner->sign(secp, message));
  expectEncodeAndReencode<Signature>(
      signed_message.signature,
      "58420142d60b3b9f27116ae24c46be6da33d310e46a2457b8dce00c73dd9e80e779c3752ba94856d4efdc39c7a61b9ed939bf1832206e4a578bb3f649fe2af3ab1495401"_unhex);
}
