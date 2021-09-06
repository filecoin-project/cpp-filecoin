/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage_market_fixture.hpp"
#include "testutil/outcome.hpp"
#include "testutil/read_file.hpp"
#include "testutil/resources/resources.hpp"

namespace fc::markets::storage::test {
  using testing::_;

  /**
   * @given provider and client
   * @when client send deal proposal, then send data in manual mode
   * @then deal activated
   */
  TEST_F(StorageMarketTest, Deal) {
    EXPECT_CALL(*chain_events_, onDealSectorCommitted(_, _, _))
        // one for client and one for provider
        .Times(2)
        .WillRepeatedly(
            testing::Invoke([](auto arg1, auto arg2, auto cb) { cb(); }));

    EXPECT_OUTCOME_TRUE(data_ref, makeDataRef(CAR_FROM_PAYLOAD_FILE));
    ChainEpoch start_epoch{210};
    ChainEpoch end_epoch{300};
    TokenAmount client_price{20000};
    TokenAmount collateral{10};
    EXPECT_OUTCOME_TRUE(proposal_cid,
                        client->proposeStorageDeal(client_id_address,
                                                   *storage_provider_info,
                                                   data_ref,
                                                   start_epoch,
                                                   end_epoch,
                                                   client_price,
                                                   collateral,
                                                   registered_proof,
                                                   false,
                                                   false));
    EXPECT_TRUE(waitForProviderDealStatus(
        proposal_cid, StorageDealStatus::STORAGE_DEAL_WAITING_FOR_DATA));
    EXPECT_TRUE(waitForClientDealStatus(
        proposal_cid, StorageDealStatus::STORAGE_DEAL_VALIDATING));

    EXPECT_OUTCOME_TRUE_1(
        provider->importDataForDeal(proposal_cid, CAR_FROM_PAYLOAD_FILE));

    EXPECT_TRUE(waitForProviderDealStatus(
        proposal_cid, StorageDealStatus::STORAGE_DEAL_ACTIVE));
    EXPECT_TRUE(waitForClientDealStatus(
        proposal_cid, StorageDealStatus::STORAGE_DEAL_ACTIVE));
  }

  /**
   * @given provider
   * @when client send deal proposal with wrong signature
   * @then state deal rejected in provider
   */
  TEST_F(StorageMarketTest, WrongSignedDealProposal) {
    node_api->WalletVerify =
        api::wrapCb([](const Address &address,
                       const Buffer &buffer,
                       const Signature &signature) -> outcome::result<bool> {
          return outcome::success(false);
        });

    EXPECT_OUTCOME_TRUE(data_ref, makeDataRef(CAR_FROM_PAYLOAD_FILE));
    ChainEpoch start_epoch{210};
    ChainEpoch end_epoch{300};
    TokenAmount client_price{20000};
    TokenAmount collateral{10};
    EXPECT_OUTCOME_TRUE(proposal_cid,
                        client->proposeStorageDeal(client_id_address,
                                                   *storage_provider_info,
                                                   data_ref,
                                                   start_epoch,
                                                   end_epoch,
                                                   client_price,
                                                   collateral,
                                                   registered_proof,
                                                   false,
                                                   false));
    EXPECT_TRUE(waitForProviderDealStatus(
        proposal_cid, StorageDealStatus::STORAGE_DEAL_ERROR));
    EXPECT_TRUE(waitForClientDealStatus(proposal_cid,
                                        StorageDealStatus::STORAGE_DEAL_ERROR));
  }

  /**
   * @given provider and client don't have enough funds
   * @when client initiates deal and waits for funding
   * @then when funding completed, proposal sent and deal activated
   */
  TEST_F(StorageMarketTest, WaitFundingDeal) {
    EXPECT_CALL(*chain_events_, onDealSectorCommitted(_, _, _))
        // one for client and one for provider
        .Times(2)
        .WillRepeatedly(
            testing::Invoke([](auto arg1, auto arg2, auto cb) { cb(); }));

    // some unique valid CID of funding message
    CID client_funding_cid = "010001020002"_cid;
    CID provider_funding_cid = "010001020003"_cid;
    node_api->MarketReserveFunds = api::wrapCb(
        [this, client_funding_cid, provider_funding_cid](
            auto wallet, auto address, auto amount) -> boost::optional<CID> {
          boost::optional<CID> result = boost::none;
          if (address == client_id_address) result = client_funding_cid;
          if (address == miner_actor_address) result = provider_funding_cid;
          if (result)
            this->logger->debug("Funding message sent "
                                + result->toString().value());
          return result;
        });

    EXPECT_OUTCOME_TRUE(data_ref, makeDataRef(CAR_FROM_PAYLOAD_FILE));
    ChainEpoch start_epoch{210};
    ChainEpoch end_epoch{300};
    TokenAmount client_price{20000};
    TokenAmount collateral{10};
    EXPECT_OUTCOME_TRUE(proposal_cid,
                        client->proposeStorageDeal(client_id_address,
                                                   *storage_provider_info,
                                                   data_ref,
                                                   start_epoch,
                                                   end_epoch,
                                                   client_price,
                                                   collateral,
                                                   registered_proof,
                                                   false,
                                                   false));
    EXPECT_TRUE(waitForProviderDealStatus(
        proposal_cid, StorageDealStatus::STORAGE_DEAL_WAITING_FOR_DATA));
    EXPECT_TRUE(waitForClientDealStatus(
        proposal_cid, StorageDealStatus::STORAGE_DEAL_VALIDATING));

    EXPECT_OUTCOME_TRUE_1(
        provider->importDataForDeal(proposal_cid, CAR_FROM_PAYLOAD_FILE));

    EXPECT_TRUE(waitForProviderDealStatus(
        proposal_cid, StorageDealStatus::STORAGE_DEAL_ACTIVE));
    EXPECT_TRUE(waitForClientDealStatus(
        proposal_cid, StorageDealStatus::STORAGE_DEAL_ACTIVE));
  }

  /**
   * @given client sends deal with incorrect transfer type
   * @when provider accepts deal and start deal data transfer
   * @then error returned, deal isn't activated
   */
  TEST_F(StorageMarketTest, WrongTransferType) {
    EXPECT_OUTCOME_TRUE(data_ref, makeDataRef(CAR_FROM_PAYLOAD_FILE));
    data_ref.transfer_type = "wrong_transfer_type";
    ChainEpoch start_epoch{210};
    ChainEpoch end_epoch{300};
    TokenAmount client_price{20000};
    TokenAmount collateral{10};
    EXPECT_OUTCOME_TRUE(proposal_cid,
                        client->proposeStorageDeal(client_id_address,
                                                   *storage_provider_info,
                                                   data_ref,
                                                   start_epoch,
                                                   end_epoch,
                                                   client_price,
                                                   collateral,
                                                   registered_proof,
                                                   false,
                                                   false));

    EXPECT_TRUE(waitForProviderDealStatus(
        proposal_cid, StorageDealStatus::STORAGE_DEAL_ERROR));
    EXPECT_OUTCOME_TRUE(provider_deal_state, provider->getDeal(proposal_cid));
    EXPECT_EQ(provider_deal_state.message,
              "Wrong transfer type: 'wrong_transfer_type'");

    clientWaitsForProviderResponse();
    EXPECT_TRUE(waitForClientDealStatus(proposal_cid,
                                        StorageDealStatus::STORAGE_DEAL_ERROR));
    EXPECT_OUTCOME_TRUE(client_deal_state, client->getLocalDeal(proposal_cid));
    EXPECT_EQ(client_deal_state.message,
              "Wrong transfer type: 'wrong_transfer_type'");
  }

  /**
   * @given provider and client
   * @when client send deal proposal, then send data in graphsync mode
   * @then deal activated
   */
  TEST_F(StorageMarketTest, GraphsyncDatatransfer) {
    EXPECT_CALL(*chain_events_, onDealSectorCommitted(_, _, _))
        // one for client and one for provider
        .Times(2)
        .WillRepeatedly(
            testing::Invoke([](auto arg1, auto arg2, auto cb) { cb(); }));

    EXPECT_OUTCOME_TRUE(data_ref, makeDataRef(CAR_FROM_PAYLOAD_FILE));
    data_ref.transfer_type = kTransferTypeGraphsync;
    ChainEpoch start_epoch{210};
    ChainEpoch end_epoch{300};
    TokenAmount client_price{20000};
    TokenAmount collateral{10};
    EXPECT_OUTCOME_TRUE(proposal_cid,
                        client->proposeStorageDeal(client_id_address,
                                                   *storage_provider_info,
                                                   data_ref,
                                                   start_epoch,
                                                   end_epoch,
                                                   client_price,
                                                   collateral,
                                                   registered_proof,
                                                   false,
                                                   false));
    EXPECT_TRUE(waitForProviderDealStatus(
        proposal_cid, StorageDealStatus::STORAGE_DEAL_ACTIVE));
    EXPECT_TRUE(waitForClientDealStatus(
        proposal_cid, StorageDealStatus::STORAGE_DEAL_ACTIVE));
  }

}  // namespace fc::markets::storage::test
