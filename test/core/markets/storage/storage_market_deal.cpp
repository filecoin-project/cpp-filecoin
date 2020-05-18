/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <future>
#include "storage_market_fixture.hpp"
#include "testutil/outcome.hpp"
#include "testutil/read_file.hpp"
#include "testutil/resources/resources.hpp"

namespace fc::markets::storage::test {

  /**
   * @given provider and client
   * @when client send deal proposal, then send data
   * @then deal activated
   */
  TEST_F(StorageMarketTest, Deal) {
    spdlog::set_level(spdlog::level::debug);

    CID root_cid = "010001020001"_cid;
    auto data = readFile(CAR_FROM_PAYLOAD_FILE);
    EXPECT_OUTCOME_TRUE(data_ref, makeDataRef(root_cid, data));
    ChainEpoch start_epoch{10};
    ChainEpoch end_epoch{200};
    TokenAmount client_price{10};
    TokenAmount collateral{10};
    EXPECT_OUTCOME_TRUE(proposal_res,
                        client->proposeStorageDeal(client_id_address,
                                                   *storage_provider_info,
                                                   data_ref,
                                                   start_epoch,
                                                   end_epoch,
                                                   client_price,
                                                   collateral,
                                                   registered_proof));
    CID proposal_cid = proposal_res.proposal_cid;

    waitForProviderDealStatus(proposal_cid,
                              StorageDealStatus::STORAGE_DEAL_WAITING_FOR_DATA);
    EXPECT_OUTCOME_TRUE_1(provider->importDataForDeal(proposal_cid, data));

    waitForProviderDealStatus(proposal_cid,
                              StorageDealStatus::STORAGE_DEAL_COMPLETED);
    EXPECT_OUTCOME_TRUE(deal, provider->getDeal(proposal_cid));
    EXPECT_EQ(deal->state, StorageDealStatus::STORAGE_DEAL_COMPLETED);
  }

}  // namespace fc::markets::storage::test
