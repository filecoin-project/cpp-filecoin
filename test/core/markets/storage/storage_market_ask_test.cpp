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
    EXPECT_OUTCOME_TRUE_1(stored_ask->addAsk(provider_price, duration));

    std::promise<outcome::result<SignedStorageAsk>> promise_ask_res;
    client->getAsk(*storage_provider_info,
                   [&](outcome::result<SignedStorageAsk> ask_res) {
                     promise_ask_res.set_value(ask_res);
                   });
    auto future = promise_ask_res.get_future();
    waitForAskResponse(future);
    auto ask_res = future.get();
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
    node_api->WalletVerify = {
        [](const Address &address,
           const Bytes &buffer,
           const Signature &signature) -> outcome::result<bool> {
          return outcome::success(false);
        }};

    TokenAmount provider_price = 1334;
    ChainEpoch duration = 2334;
    EXPECT_OUTCOME_TRUE_1(stored_ask->addAsk(provider_price, duration));

    std::promise<outcome::result<SignedStorageAsk>> promise_ask_res;
    client->getAsk(*storage_provider_info,
                   [&](outcome::result<SignedStorageAsk> ask_res) {
                     promise_ask_res.set_value(ask_res);
                   });
    auto future = promise_ask_res.get_future();
    waitForAskResponse(future);
    EXPECT_OUTCOME_ERROR(StorageMarketClientError::kSignatureInvalid,
                         future.get());
  }

}  // namespace fc::markets::storage::test
