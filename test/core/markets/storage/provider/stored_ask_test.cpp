/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "markets/storage/provider/stored_ask.hpp"

#include <gtest/gtest.h>
#include "crypto/bls/impl/bls_provider_impl.hpp"
#include "crypto/secp256k1/impl/secp256k1_sha256_provider_impl.hpp"
#include "storage/in_memory/in_memory_storage.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"

namespace fc::markets::storage::provider {

  using api::MinerInfo;
  using api::TipsetKey;
  using crypto::signature::Signature;
  using fc::crypto::bls::BlsProvider;
  using fc::crypto::bls::BlsProviderImpl;
  using fc::crypto::bls::KeyPair;
  using fc::crypto::secp256k1::Secp256k1ProviderDefault;
  using fc::crypto::secp256k1::Secp256k1Sha256ProviderImpl;
  using fc::storage::InMemoryStorage;
  using BlsSignature = fc::crypto::bls::Signature;

  class StoredAskTest : public ::testing::Test {
   public:
    std::shared_ptr<BlsProvider> bls_provider_ =
        std::make_shared<BlsProviderImpl>();
    std::shared_ptr<Secp256k1ProviderDefault> secp256k1_provider_ =
        std::make_shared<Secp256k1Sha256ProviderImpl>();

    std::shared_ptr<Datastore> datastore = std::make_shared<InMemoryStorage>();

    ChainEpoch epoch = 100;
    std::shared_ptr<FullNodeApi> api = std::make_shared<FullNodeApi>();
    std::shared_ptr<const Tipset> chain_head;
    Address actor_address = Address::makeFromId(1);
    Address bls_address;
    KeyPair bls_keypair;
    StoredAsk stored_ask{datastore, api, actor_address};

    static fc::primitives::block::BlockHeader makeBlock(ChainEpoch epoch) {
      auto bls1 =
          "010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101"_blob96;

      return fc::primitives::block::BlockHeader{
          fc::primitives::address::Address::makeFromId(1),
          fc::primitives::block::Ticket{copy(bls1)},
          {},
          {fc::primitives::block::BeaconEntry{
              4,
              "F00D"_unhex,
          }},
          {fc::primitives::sector::PoStProof{
              fc::primitives::sector::RegisteredPoStProof::
                  kStackedDRG2KiBWinningPoSt,
              "F00D"_unhex,
          }},
          {CbCid::hash("01"_unhex)},
          fc::primitives::BigInt(3),
          epoch,
          "010001020005"_cid,
          "010001020006"_cid,
          "010001020007"_cid,
          boost::none,
          8,
          boost::none,
          9,
          {},
      };
    }

    void SetUp() override {
      chain_head = Tipset::create({makeBlock(epoch)}).value();

      api->ChainHead = {[=]() { return chain_head; }};

      bls_keypair = bls_provider_->generateKeyPair().value();
      bls_address = Address::makeBls(bls_keypair.public_key);
      api->WalletSign = {
          [=](const Address &address,
              const Bytes &bytes) -> outcome::result<Signature> {
            if (address != bls_address) throw "API WalletSign: Wrong address";
            return Signature{
                bls_provider_->sign(bytes, bls_keypair.private_key).value()};
          }};

      api->StateMinerInfo = {
          [=](const Address &,
              const TipsetKey &) -> outcome::result<MinerInfo> {
            MinerInfo info;
            info.worker = actor_address;
            return info;
          }};
      api->StateAccountKey = {[=](auto &address, auto &tipset_key) {
        if (address == actor_address) return bls_address;
        throw "API StateAccountKey: Unexpected address";
      }};
    }
  };

  /**
   * @given empty datastore
   * @when get stored ask
   * @then default stored ask returned
   */
  TEST_F(StoredAskTest, DefaultAsk) {
    EXPECT_OUTCOME_TRUE(ask, stored_ask.getAsk(actor_address));

    EXPECT_EQ(ask.ask.price, kDefaultPrice);
    EXPECT_EQ(ask.ask.min_piece_size, kDefaultMinPieceSize);
    EXPECT_EQ(ask.ask.max_piece_size, kDefaultMaxPieceSize);
    EXPECT_EQ(ask.ask.miner, actor_address);
    EXPECT_EQ(ask.ask.timestamp, epoch);
    EXPECT_EQ(ask.ask.expiry, epoch + kDefaultDuration);
    EXPECT_EQ(ask.ask.seq_no, 0);
    EXPECT_OUTCOME_TRUE(verify_data, codec::cbor::encode(ask.ask));
    EXPECT_OUTCOME_EQ(
        bls_provider_->verifySignature(verify_data,
                                       boost::get<BlsSignature>(ask.signature),
                                       bls_keypair.public_key),
        true);
  }

  /**
   * @given added ask
   * @when get stored ask
   * @then stored ask returned
   */
  TEST_F(StoredAskTest, AddAsk) {
    TokenAmount price = 1334;
    ChainEpoch duration = 2445;
    EXPECT_OUTCOME_TRUE_1(stored_ask.addAsk(price, duration));

    EXPECT_OUTCOME_TRUE(ask, stored_ask.getAsk(actor_address));

    EXPECT_EQ(ask.ask.price, price);
    EXPECT_EQ(ask.ask.min_piece_size, kDefaultMinPieceSize);
    EXPECT_EQ(ask.ask.max_piece_size, kDefaultMaxPieceSize);
    EXPECT_EQ(ask.ask.miner, actor_address);
    EXPECT_EQ(ask.ask.timestamp, epoch);
    EXPECT_EQ(ask.ask.expiry, epoch + duration);
    EXPECT_EQ(ask.ask.seq_no, 0);
    EXPECT_OUTCOME_TRUE(verify_data, codec::cbor::encode(ask.ask));
    EXPECT_OUTCOME_EQ(
        bls_provider_->verifySignature(verify_data,
                                       boost::get<BlsSignature>(ask.signature),
                                       bls_keypair.public_key),
        true);
  }

  /**
   * @given added ask
   * @when add ask again
   * @then stored ask returned and seqno incremented
   */
  TEST_F(StoredAskTest, AddAskTwoTimes) {
    TokenAmount price = 1334;
    ChainEpoch duration = 2445;
    EXPECT_OUTCOME_TRUE_1(stored_ask.addAsk(price, duration));
    EXPECT_OUTCOME_TRUE_1(stored_ask.addAsk(price, duration));

    EXPECT_OUTCOME_TRUE(ask, stored_ask.getAsk(actor_address));

    EXPECT_EQ(ask.ask.price, price);
    EXPECT_EQ(ask.ask.min_piece_size, kDefaultMinPieceSize);
    EXPECT_EQ(ask.ask.max_piece_size, kDefaultMaxPieceSize);
    EXPECT_EQ(ask.ask.miner, actor_address);
    EXPECT_EQ(ask.ask.timestamp, epoch);
    EXPECT_EQ(ask.ask.expiry, epoch + duration);
    EXPECT_EQ(ask.ask.seq_no, 1);
    EXPECT_OUTCOME_TRUE(verify_data, codec::cbor::encode(ask.ask));
    EXPECT_OUTCOME_EQ(
        bls_provider_->verifySignature(verify_data,
                                       boost::get<BlsSignature>(ask.signature),
                                       bls_keypair.public_key),
        true);
  }

  /**
   * @given stored ask with actor_address
   * @when call getAsk with wrong address
   * @then error returned
   */
  TEST_F(StoredAskTest, WrongAddress) {
    Address wrong_address = Address::makeFromId(2);
    EXPECT_OUTCOME_ERROR(StoredAskError::kWrongAddress,
                         stored_ask.getAsk(wrong_address));
  }

  /**
   * @given added ask in store and new stored ask created
   * @when get ask called
   * @then stored ask returned
   */
  TEST_F(StoredAskTest, LoadStoredAsk) {
    TokenAmount price = 1334;
    ChainEpoch duration = 2445;
    EXPECT_OUTCOME_TRUE_1(stored_ask.addAsk(price, duration));

    StoredAsk fresh_stored_ask{datastore, api, actor_address};

    EXPECT_OUTCOME_TRUE(ask, fresh_stored_ask.getAsk(actor_address));

    EXPECT_EQ(ask.ask.price, price);
    EXPECT_EQ(ask.ask.min_piece_size, kDefaultMinPieceSize);
    EXPECT_EQ(ask.ask.max_piece_size, kDefaultMaxPieceSize);
    EXPECT_EQ(ask.ask.miner, actor_address);
    EXPECT_EQ(ask.ask.timestamp, epoch);
    EXPECT_EQ(ask.ask.expiry, epoch + duration);
    EXPECT_EQ(ask.ask.seq_no, 0);
    EXPECT_OUTCOME_TRUE(verify_data, codec::cbor::encode(ask.ask));
    EXPECT_OUTCOME_EQ(
        bls_provider_->verifySignature(verify_data,
                                       boost::get<BlsSignature>(ask.signature),
                                       bls_keypair.public_key),
        true);
  }

}  // namespace fc::markets::storage::provider
