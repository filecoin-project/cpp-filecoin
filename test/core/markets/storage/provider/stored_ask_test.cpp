/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "markets/storage/provider/stored_ask.hpp"

#include <gtest/gtest.h>
#include "crypto/bls/impl/bls_provider_impl.hpp"
#include "crypto/secp256k1/impl/secp256k1_sha256_provider_impl.hpp"
#include "storage/in_memory/in_memory_storage.hpp"
#include "testutil/outcome.hpp"

namespace fc::markets::storage::provider {

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
    std::shared_ptr<Api> api = std::make_shared<Api>();
    Tipset chain_head;
    Address actor_address = Address::makeFromId(1);
    Address bls_address;
    KeyPair bls_keypair;
    StoredAsk stored_ask{datastore, api, actor_address};

    void SetUp() override {
      // TODO generate valid chain_head with proper height
      //chain_head.height = epoch;

      // BlockHeader { .... height = epoch, ... }
      // chain_head = Tipset::create( { header} ).value();

      api->ChainHead = {[=]() { return chain_head; }};

      bls_keypair = bls_provider_->generateKeyPair().value();
      bls_address = Address::makeBls(bls_keypair.public_key);
      api->WalletSign = {
          [=](const Address &address,
              const Buffer &buffer) -> outcome::result<Signature> {
            if (address != bls_address) throw "API WalletSign: Wrong address";
            return Signature{
                bls_provider_->sign(buffer, bls_keypair.private_key).value()};
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
    EXPECT_OUTCOME_ERROR(StoredAskError::WRONG_ADDRESS,
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
