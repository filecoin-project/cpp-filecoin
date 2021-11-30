/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <gtest/gtest.h>

#include "crypto/bls/impl/bls_provider_impl.hpp"
#include "testutil/cbor.hpp"
#include "vm/actor/builtin/types/market/deal.hpp"

namespace fc::markets::storage {
  using crypto::bls::BlsProviderImpl;
  using crypto::signature::Signature;
  using primitives::address::Address;
  using primitives::address::decodeFromString;
  using primitives::piece::PaddedPieceSize;
  using primitives::piece::UnpaddedPieceSize;
  using testutil::codec::cbor::normalizeMap;
  using vm::actor::builtin::types::market::DealProposal;

  class ProtocolNamedCborTestFixture : public ::testing::Test {
   public:
    // address from go test constants
    Address address =
        decodeFromString("t2i4llai5x72clnz643iydyplvjmni74x4vyme7ny").value();
    // CID from go-generated string
    CID cid = CID::fromString("QmTTA2daxGqo5denp6SwLzzkLJm3fuisYEi9CoWsuHpzfb")
                  .value();

    DealProposal deal_proposal{
        .piece_cid = cid,
        .piece_size = PaddedPieceSize{256},
        .verified = true,
        .client = address,
        .provider = address,
        .label = "label",
        .start_epoch = 101,
        .end_epoch = 2002,
        .storage_price_per_epoch = 22,
        .provider_collateral = 333,
        .client_collateral = 4444,
    };

    std::vector<uint8_t> privkey_bytes{
        "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f"_unhex};
    crypto::bls::PrivateKey private_key;
    crypto::bls::PublicKey public_key;
    crypto::bls::BlsProviderImpl bls_provider;

    void SetUp() override {
      std::copy(
          privkey_bytes.begin(), privkey_bytes.end(), private_key.begin());
      EXPECT_OUTCOME_TRUE(pubkey, bls_provider.derivePublicKey(private_key));
      public_key = pubkey;
    }

    template <typename Signable>
    void sign(Signable &to_sign) const {
      EXPECT_OUTCOME_TRUE(digest, to_sign.getDigest());
      EXPECT_OUTCOME_TRUE(signature, bls_provider.sign(digest, private_key));
      to_sign.signature = signature;
    }

    template <typename Signable>
    void verify(const Signable &to_sign) const {
      EXPECT_OUTCOME_TRUE(digest, to_sign.getDigest());
      EXPECT_OUTCOME_EQ(
          bls_provider.verifySignature(
              digest,
              boost::get<crypto::bls::Signature>(to_sign.signature),
              public_key),
          true);
    }
  };

}  // namespace fc::markets::storage
