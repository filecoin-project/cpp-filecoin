/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>
#include <rapidjson/writer.h>
#include <boost/filesystem.hpp>

#include "api/rpc/json.hpp"
#include "common/span.hpp"
#include "crypto/bls/impl/bls_provider_impl.hpp"
#include "storage/keystore/impl/in_memory/in_memory_keystore.hpp"
#include "testutil/default_print.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"
#include "testutil/read_file.hpp"
#include "testutil/resources/resources.hpp"
#include "vm/message/impl/message_signer_impl.hpp"

using fc::api::BlockHeader;
namespace codec = fc::codec::json;
using fc::api::Signature;
using fc::api::UnsignedMessage;
using fc::common::hex_lower;
using fc::crypto::bls::BlsProviderImpl;
using fc::storage::keystore::InMemoryKeyStore;
using fc::vm::message::MessageSignerImpl;
using BlsPrivateKey = fc::crypto::bls::PrivateKey;

auto loadJson(const std::string &name) {
  auto str =
      readFile(resourcePath("serialization/serialization_vectors") / name);
  rapidjson::Document document;
  document.Parse(fc::common::span::cstring(str).data(), str.size());
  EXPECT_FALSE(document.HasParseError());
  return document;
}

auto signMessage(const UnsignedMessage &message, const BlsPrivateKey &key) {
  auto keystore = std::make_shared<InMemoryKeyStore>(
      std::make_shared<BlsProviderImpl>(), nullptr);
  EXPECT_OUTCOME_TRUE_1(keystore->put(message.from, key));
  MessageSignerImpl signer{keystore};
  EXPECT_OUTCOME_TRUE(sig, signer.sign(message.from, message));
  return sig.signature;
}

/// Message JSON and CBOR
TEST(SerializationVectorsTest, DISABLED_UnsignedMessage) {
  auto tests = loadJson("unsigned_messages.json");
  for (auto it = tests.Begin(); it != tests.End(); ++it) {
    auto message = codec::innerDecode<UnsignedMessage>(codec::Get(*it, "message"));
    auto expected_cbor = codec::AsString(codec::Get(*it, "hex_cbor"));
    EXPECT_OUTCOME_TRUE(cbor, fc::codec::cbor::encode(message));
    EXPECT_EQ(hex_lower(cbor), expected_cbor);
  }
}

/// Message signing
TEST(SerializationVectorsTest, DISABLED_SignedMessage) {
  auto tests = loadJson("message_signing.json");
  for (auto it = tests.Begin(); it != tests.End(); ++it) {
    auto message = codec::innerDecode<UnsignedMessage>(codec::Get(*it, "Unsigned"));
    auto cid{message.getCid()};
    EXPECT_OUTCOME_TRUE(cid_bytes, cid.toBytes());

    auto expected_cid_hex = codec::AsString(codec::Get(*it, "CidHexBytes"));
    EXPECT_EQ(hex_lower(cid_bytes), expected_cid_hex);

    auto expected_cid_str = codec::AsString(codec::Get(*it, "Cid"));
    EXPECT_OUTCOME_EQ(cid.toString(), expected_cid_str);

    auto expected_sig = codec::innerDecode<Signature>(codec::Get(*it, "Signature"));
    EXPECT_TRUE(expected_sig.isBls());

    auto key = codec::innerDecode<BlsPrivateKey>(codec::Get(*it, "PrivateKey"));
    auto sig = signMessage(message, key);

    EXPECT_EQ(sig, expected_sig);
  }
}

/// BlockHeader JSON and CBOR
TEST(SerializationVectorsTest, DISABLED_BlockHeader) {
  auto tests = loadJson("block_headers.json");
  for (auto it = tests.Begin(); it != tests.End(); ++it) {
    auto block = codec::innerDecode<BlockHeader>(codec::Get(*it, "block"));

    EXPECT_OUTCOME_TRUE(cbor, fc::codec::cbor::encode(block));
    auto expected_cbor = codec::AsString(codec::Get(*it, "cbor_hex"));
    EXPECT_EQ(hex_lower(cbor), expected_cbor);

    EXPECT_OUTCOME_TRUE(cid, fc::common::getCidOf(cbor));
    auto expected_cid_str = codec::AsString(codec::Get(*it, "cid"));
    EXPECT_OUTCOME_EQ(cid.toString(), expected_cid_str);
  }
}
