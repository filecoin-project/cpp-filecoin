/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <future>
#include "markets/storage/provider/stored_ask.hpp"
#include "storage_market_fixture.hpp"
#include "testutil/outcome.hpp"

namespace fc::markets::storage::test {
  using provider::kDefaultMaxPieceSize;
  using provider::kDefaultMinPieceSize;

  /**
   * @given provider with ask
   * @when client send get ask
   * @then ask returned in answer
   */
  TEST_F(StorageMarketTest, Ask) {
    spdlog::set_level(spdlog::level::debug);

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
    EXPECT_EQ(ask_res.value().ask.timestamp, chain_head.height);
    EXPECT_EQ(ask_res.value().ask.expiry, chain_head.height + duration);
    EXPECT_EQ(ask_res.value().ask.seq_no, 0);
  }

}  // namespace fc::markets::storage::test
