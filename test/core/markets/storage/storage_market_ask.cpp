/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <future>
#include "markets/storage/provider/stored_ask.hpp"
#include "storage_market_fixture.hpp"
#include "testutil/outcome.hpp"

namespace fc::markets::storage::test {
  using client::StorageMarketClientError;
  using provider::kDefaultMaxPieceSize;
  using provider::kDefaultMinPieceSize;

  /**
   * @given provider with ask
   * @when client send get ask
   * @then ask returned in answer
   */
  TEST_F(StorageMarketTest, Ask) {
    TokenAmount provider_price = 1334;
    ChainEpoch duration = 2334;
    EXPECT_OUTCOME_TRUE_1(provider->addAsk(provider_price, duration));

    std::promise<outcome::result<SignedStorageAsk>> promise_ask_res;
    client->getAsk(*storage_provider_info,
                   [&](outcome::result<SignedStorageAsk> ask_res) {
                     promise_ask_res.set_value(ask_res);
                   });
    // wait for result
    auto ask_res = promise_ask_res.get_future().get();
    EXPECT_TRUE(ask_res.has_value());
    EXPECT_EQ(ask_res.value().ask.price, provider_price);
    EXPECT_EQ(ask_res.value().ask.min_piece_size, kDefaultMinPieceSize);
    EXPECT_EQ(ask_res.value().ask.max_piece_size, kDefaultMaxPieceSize);
    EXPECT_EQ(ask_res.value().ask.miner, storage_provider_info->address);
    EXPECT_EQ(ask_res.value().ask.timestamp, chain_head->height());
    EXPECT_EQ(ask_res.value().ask.expiry, chain_head->height() + duration);
    EXPECT_EQ(ask_res.value().ask.seq_no, 0);
  }

  /**
   * @given provider with ask with wrong signature
   * @when client send get ask
   * @then result with error wrong signature
   */
  TEST_F(StorageMarketTest, WrongSignedAsk) {
    std::shared_ptr<BlsProvider> bls_provider =
        std::make_shared<BlsProviderImpl>();
    OUTCOME_EXCEPT(wrong_keypair, bls_provider->generateKeyPair());

    node_api->WalletSign = {
        [this, wrong_keypair, bls_provider](
            const Address &address,
            const Buffer &buffer) -> outcome::result<Signature> {
          if (address == this->miner_worker_address)
            return Signature{
                bls_provider->sign(buffer, wrong_keypair.private_key).value()};
          throw "API WalletSign: address not found";
        }};

    TokenAmount provider_price = 1334;
    ChainEpoch duration = 2334;
    EXPECT_OUTCOME_TRUE_1(provider->addAsk(provider_price, duration));

    std::promise<outcome::result<SignedStorageAsk>> promise_ask_res;
    client->getAsk(*storage_provider_info,
                   [&](outcome::result<SignedStorageAsk> ask_res) {
                     promise_ask_res.set_value(ask_res);
                   });
    // wait for result
    EXPECT_OUTCOME_ERROR(StorageMarketClientError::SIGNATURE_INVALID,
                         promise_ask_res.get_future().get());
  }

}  // namespace fc::markets::storage::test
