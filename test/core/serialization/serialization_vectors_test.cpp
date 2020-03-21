/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "serialization_vectors.hpp"

#include <fstream>

#include <gtest/gtest.h>
#include <rapidjson/writer.h>

#include "api/rpc/json.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"
#include "vm/message/impl/message_signer_impl.hpp"
#include "vm/message/message_util.hpp"
#include "storage/keystore/impl/in_memory/in_memory_keystore.hpp"
#include "crypto/bls/impl/bls_provider_impl.hpp"

using fc::api::Codec;
using fc::api::UnsignedMessage;
using fc::api::Signature;
using fc::api::SignatureType;
using fc::common::hex_lower;
using fc::storage::keystore::InMemoryKeyStore;
using fc::vm::message::MessageSignerImpl;
using fc::crypto::bls::BlsProviderImpl;
using BlsPrivateKey = fc::crypto::bls::PrivateKey;

auto loadJson(const std::string &name) {
  std::ifstream file{
      std::filesystem::path{SERIALIZATION_VECTORS_ROOT}.append(name),
      std::ios::binary | std::ios::ate};
  EXPECT_TRUE(file.good());
  std::string str;
  str.resize(file.tellg());
  file.seekg(0, std::ios::beg);
  file.read(str.data(), str.size());
  rapidjson::Document document;
  document.Parse(str.data(), str.size());
  EXPECT_FALSE(document.HasParseError());
  return document;
}

auto signMessage(const UnsignedMessage &message, const BlsPrivateKey &key) {
  auto keystore = std::make_shared<InMemoryKeyStore>(std::make_shared<BlsProviderImpl>(), nullptr);
  EXPECT_OUTCOME_TRUE_1(keystore->put(message.from, key));
  MessageSignerImpl signer{keystore};
  EXPECT_OUTCOME_TRUE(sig, signer.sign(message.from, message));
  return sig.signature;
}

TEST(SerializationVectorsTest, UnsignedMessage) {
  auto tests = loadJson("unsigned_messages.json");
  for (auto it = tests.Begin(); it != tests.End(); ++it) {
    auto message = Codec::decode<UnsignedMessage>(Codec::Get(*it, "message"));
    auto expected_cbor = Codec::AsString(Codec::Get(*it, "hex_cbor"));
    EXPECT_OUTCOME_TRUE(cbor, fc::codec::cbor::encode(message));
    EXPECT_EQ(hex_lower(cbor), expected_cbor);
  }
}

TEST(SerializationVectorsTest, SignedMessage) {
  auto tests = loadJson("message_signing.json");
  for (auto it = tests.Begin(); it != tests.End(); ++it) {
    auto message = Codec::decode<UnsignedMessage>(Codec::Get(*it, "Unsigned"));
    EXPECT_OUTCOME_TRUE(cid, fc::vm::message::cid(message));
    EXPECT_OUTCOME_TRUE(cid_bytes, cid.toBytes());

    auto expected_cid_hex = Codec::AsString(Codec::Get(*it, "CidHexBytes"));
    EXPECT_EQ(hex_lower(cid_bytes), expected_cid_hex);

    auto expected_cid_str = Codec::AsString(Codec::Get(*it, "Cid"));
    EXPECT_OUTCOME_EQ(cid.toString(), expected_cid_str);

    auto expected_sig = Codec::decode<Signature>(Codec::Get(*it, "Signature"));
    EXPECT_EQ(fc::crypto::signature::typeCode(expected_sig), SignatureType::BLS);

    auto key = Codec::decode<BlsPrivateKey>(Codec::Get(*it, "PrivateKey"));
    auto sig = signMessage(message, key);

    EXPECT_EQ(sig, expected_sig);
  }
}
