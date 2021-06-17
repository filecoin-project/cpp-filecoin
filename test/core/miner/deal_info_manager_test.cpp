/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "miner/storage_fsm/impl/deal_info_manager_impl.hpp"

#include <gtest/gtest.h>
#include <vm/message/message.hpp>

#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"
#include "vm/actor/builtin/v0/market/market_actor.hpp"
#include "vm/exit_code/exit_code.hpp"

namespace fc::mining {
  using api::Address;
  using api::MsgWait;
  using vm::VMExitCode;
  using vm::actor::builtin::types::market::ClientDealProposal;
  using vm::actor::builtin::v0::market::PublishStorageDeals;
  using vm::message::UnsignedMessage;

  class DealInfoManagerTest : public testing::Test {
   protected:
    virtual void SetUp() {
      api_ = std::make_shared<FullNodeApi>();

      manager_ = std::make_shared<DealInfoManagerImpl>(api_);
    }

    std::shared_ptr<FullNodeApi> api_;
    std::shared_ptr<DealInfoManager> manager_;
  };

  /**
   * @given publish cid
   * @when try to get current deal info, but msg search is not ok
   * @then DealInfoManagerError::kNotOkExitCode occurs
   */
  TEST_F(DealInfoManagerTest, NonOkCode) {
    CID publish_cid = "010001020001"_cid;

    api_->StateSearchMsg =
        [&publish_cid](
            const CID &cid) -> outcome::result<boost::optional<MsgWait>> {
      if (cid == publish_cid) {
        MsgWait lookup;
        lookup.receipt.exit_code = VMExitCode::kFatal;
        return lookup;
      }
      return ERROR_TEXT("ERROR");
    };

    EXPECT_OUTCOME_ERROR(
        DealInfoManagerError::kNotOkExitCode,
        manager_->getCurrentDealInfo(TipsetKey(), boost::none, publish_cid));
  }

  /**
   * @given publish cid
   * @when try to get current deal info, but not proposal and more than one
   * deals
   * @then DealInfoManagerError::kMoreThanOneDeal occurs
   */
  TEST_F(DealInfoManagerTest, TwoDealsWithoutProposal) {
    CID publish_cid = "010001020001"_cid;

    api_->StateSearchMsg =
        [&publish_cid](
            const CID &cid) -> outcome::result<boost::optional<MsgWait>> {
      if (cid == publish_cid) {
        MsgWait lookup;
        lookup.receipt.exit_code = VMExitCode::kOk;
        PublishStorageDeals::Result result{
            .deals = {1, 2},
        };
        OUTCOME_TRYA(lookup.receipt.return_value, codec::cbor::encode(result));
        return lookup;
      }
      return ERROR_TEXT("ERROR");
    };

    EXPECT_OUTCOME_ERROR(
        DealInfoManagerError::kMoreThanOneDeal,
        manager_->getCurrentDealInfo(TipsetKey(), boost::none, publish_cid));
  }

  /**
   * @given publish cid, tipset key
   * @when try to get current deal info, but not proposal
   * @then success
   */
  TEST_F(DealInfoManagerTest, SuccessWithoutProposal) {
    CID publish_cid = "010001020001"_cid;

    TipsetKey key{{CbCid::hash("02"_unhex)}};
    TipsetKey result_key{{CbCid::hash("03"_unhex), CbCid::hash("04"_unhex)}};

    DealId result_deal_id = 1;

    StorageDeal market_deal;

    CurrentDealInfo result_deal{
        .deal_id = result_deal_id,
        .market_deal = market_deal,
        .publish_msg_tipset = result_key,
    };

    api_->StateSearchMsg =
        [&publish_cid, &result_deal_id, &result_key](
            const CID &cid) -> outcome::result<boost::optional<MsgWait>> {
      if (cid == publish_cid) {
        MsgWait lookup;
        lookup.receipt.exit_code = VMExitCode::kOk;
        lookup.tipset = result_key;
        PublishStorageDeals::Result result{
            .deals = {result_deal_id},
        };
        OUTCOME_TRYA(lookup.receipt.return_value, codec::cbor::encode(result));
        return lookup;
      }
      return ERROR_TEXT("ERROR");
    };

    api_->StateMarketStorageDeal =
        [&result_deal_id, &key, &market_deal](
            DealId deal_id,
            const TipsetKey &tipset_key) -> outcome::result<StorageDeal> {
      if (deal_id == result_deal_id and tipset_key == key) {
        return market_deal;
      }
      return ERROR_TEXT("ERROR");
    };

    EXPECT_OUTCOME_EQ(
        manager_->getCurrentDealInfo(key, boost::none, publish_cid),
        result_deal);
  }

  /**
   * @given publish cid, tipset key, proposal
   * @when try to get current deal info, but not found deal
   * @then DealInfoManagerError::kNotFound occurs
   */
  TEST_F(DealInfoManagerTest, NotFoundDeal) {
    CID publish_cid = "010001020001"_cid;

    DealProposal proposal;
    proposal.verified = false;
    proposal.client = Address::makeFromId(2);
    proposal.provider = Address::makeFromId(1);

    TipsetKey key{{CbCid::hash("02"_unhex)}};
    TipsetKey result_key{{CbCid::hash("03"_unhex), CbCid::hash("04"_unhex)}};

    DealId result_deal_id = 1;

    StorageDeal market_deal;

    CurrentDealInfo result_deal{
        .deal_id = result_deal_id,
        .market_deal = market_deal,
        .publish_msg_tipset = result_key,
    };

    api_->StateSearchMsg =
        [&publish_cid, &result_deal_id, &result_key](
            const CID &cid) -> outcome::result<boost::optional<MsgWait>> {
      if (cid == publish_cid) {
        MsgWait lookup;
        lookup.receipt.exit_code = VMExitCode::kOk;
        lookup.tipset = result_key;
        PublishStorageDeals::Result result{
            .deals = {result_deal_id},
        };
        OUTCOME_TRYA(lookup.receipt.return_value, codec::cbor::encode(result));
        return lookup;
      }
      return ERROR_TEXT("ERROR");
    };

    Address another_provider = Address::makeFromId(2);

    api_->ChainGetMessage =
        [&publish_cid, &another_provider](
            const CID &cid) -> outcome::result<UnsignedMessage> {
      if (cid == publish_cid) {
        UnsignedMessage result;
        DealProposal proposal;
        proposal.piece_cid = "010001020005"_cid;
        proposal.verified = false;
        proposal.client = Address::makeFromId(2);
        proposal.provider = another_provider;
        PublishStorageDeals::Params params{
            .deals = {ClientDealProposal{
                .proposal = proposal,
                .client_signature = BlsSignature(),
            }}};
        OUTCOME_TRYA(result.params, codec::cbor::encode(params));
        return result;
      }
      return ERROR_TEXT("ERROR");
    };

    api_->StateMarketStorageDeal =
        [&result_deal_id, &key, &market_deal](
            DealId deal_id,
            const TipsetKey &tipset_key) -> outcome::result<StorageDeal> {
      if (deal_id == result_deal_id and tipset_key == key) {
        return market_deal;
      }
      return ERROR_TEXT("ERROR");
    };

    api_->StateLookupID =
        [&key](const Address &address,
               const TipsetKey &tipset_key) -> outcome::result<Address> {
      if (tipset_key == key) {
        return address;
      }
      return ERROR_TEXT("ERROR");
    };

    EXPECT_OUTCOME_ERROR(
        DealInfoManagerError::kNotFound,
        manager_->getCurrentDealInfo(key, proposal, publish_cid));
  }

  /**
   * @given publish cid, tipset key, proposal
   * @when try to get current deal info, but index more than deals
   * @then DealInfoManagerError::kOutOfRange occurs
   */
  TEST_F(DealInfoManagerTest, OutOfRangeDeal) {
    CID publish_cid = "010001020001"_cid;

    DealProposal proposal;
    proposal.piece_cid = "010001020006"_cid;
    proposal.verified = false;
    proposal.client = Address::makeFromId(2);
    proposal.provider = Address::makeFromId(1);

    TipsetKey key{{CbCid::hash("02"_unhex)}};
    TipsetKey result_key{{CbCid::hash("03"_unhex), CbCid::hash("04"_unhex)}};

    DealId result_deal_id = 1;

    StorageDeal market_deal;

    CurrentDealInfo result_deal{
        .deal_id = result_deal_id,
        .market_deal = market_deal,
        .publish_msg_tipset = result_key,
    };

    api_->StateSearchMsg =
        [&publish_cid, &result_deal_id, &result_key](
            const CID &cid) -> outcome::result<boost::optional<MsgWait>> {
      if (cid == publish_cid) {
        MsgWait lookup;
        lookup.receipt.exit_code = VMExitCode::kOk;
        lookup.tipset = result_key;
        PublishStorageDeals::Result result{
            .deals = {result_deal_id},
        };
        OUTCOME_TRYA(lookup.receipt.return_value, codec::cbor::encode(result));
        return lookup;
      }
      return ERROR_TEXT("ERROR");
    };

    Address another_provider = Address::makeFromId(2);

    api_->ChainGetMessage =
        [&publish_cid, &another_provider, result_proposal{proposal}](
            const CID &cid) -> outcome::result<UnsignedMessage> {
      if (cid == publish_cid) {
        UnsignedMessage result;
        DealProposal proposal;
        proposal.verified = false;
        proposal.client = Address::makeFromId(2);
        proposal.piece_cid = "010001020005"_cid;
        proposal.provider = another_provider;
        PublishStorageDeals::Params params{
            .deals = {ClientDealProposal{
                          .proposal = proposal,
                          .client_signature = BlsSignature(),
                      },
                      ClientDealProposal{
                          .proposal = result_proposal,
                          .client_signature = BlsSignature(),
                      }}};
        OUTCOME_TRYA(result.params, codec::cbor::encode(params));
        return result;
      }
      return ERROR_TEXT("ERROR");
    };

    api_->StateMarketStorageDeal =
        [&result_deal_id, &key, &market_deal](
            DealId deal_id,
            const TipsetKey &tipset_key) -> outcome::result<StorageDeal> {
      if (deal_id == result_deal_id and tipset_key == key) {
        return market_deal;
      }
      return ERROR_TEXT("ERROR");
    };

    api_->StateLookupID =
        [&key](const Address &address,
               const TipsetKey &tipset_key) -> outcome::result<Address> {
      if (tipset_key == key) {
        return address;
      }
      return ERROR_TEXT("ERROR");
    };

    EXPECT_OUTCOME_ERROR(
        DealInfoManagerError::kOutOfRange,
        manager_->getCurrentDealInfo(key, proposal, publish_cid));
  }

  /**
   * @given publish cid, tipset key, proposal
   * @when try to get current deal info, but market proposal is another
   * @then DealInfoManagerError::kDealProposalNotMatch occurs
   */
  TEST_F(DealInfoManagerTest, NotMatchProposal) {
    CID publish_cid = "010001020001"_cid;

    DealProposal proposal;
    proposal.piece_cid = "010001020006"_cid;
    proposal.verified = false;
    proposal.client = Address::makeFromId(2);
    proposal.provider = Address::makeFromId(1);

    TipsetKey key{{CbCid::hash("02"_unhex)}};
    TipsetKey result_key{{CbCid::hash("03"_unhex), CbCid::hash("04"_unhex)}};

    DealId result_deal_id = 1;

    StorageDeal market_deal;

    CurrentDealInfo result_deal{
        .deal_id = result_deal_id,
        .market_deal = market_deal,
        .publish_msg_tipset = result_key,
    };

    api_->StateSearchMsg =
        [&publish_cid, &result_deal_id, &result_key](
            const CID &cid) -> outcome::result<boost::optional<MsgWait>> {
      if (cid == publish_cid) {
        MsgWait lookup;
        lookup.receipt.exit_code = VMExitCode::kOk;
        lookup.tipset = result_key;
        PublishStorageDeals::Result result{
            .deals = {result_deal_id},
        };
        OUTCOME_TRYA(lookup.receipt.return_value, codec::cbor::encode(result));
        return lookup;
      }
      return ERROR_TEXT("ERROR");
    };

    api_->ChainGetMessage =
        [&publish_cid,
         &proposal](const CID &cid) -> outcome::result<UnsignedMessage> {
      if (cid == publish_cid) {
        UnsignedMessage result;
        PublishStorageDeals::Params params{
            .deals = {ClientDealProposal{
                .proposal = proposal,
                .client_signature = BlsSignature(),
            }}};
        OUTCOME_TRYA(result.params, codec::cbor::encode(params));
        return result;
      }
      return ERROR_TEXT("ERROR");
    };

    api_->StateMarketStorageDeal =
        [&result_deal_id, &key, &market_deal](
            DealId deal_id,
            const TipsetKey &tipset_key) -> outcome::result<StorageDeal> {
      if (deal_id == result_deal_id and tipset_key == key) {
        return market_deal;
      }
      return ERROR_TEXT("ERROR");
    };

    api_->StateLookupID =
        [&key](const Address &address,
               const TipsetKey &tipset_key) -> outcome::result<Address> {
      if (tipset_key == key) {
        return address;
      }
      return ERROR_TEXT("ERROR");
    };

    EXPECT_OUTCOME_ERROR(
        DealInfoManagerError::kDealProposalNotMatch,
        manager_->getCurrentDealInfo(key, proposal, publish_cid));
  }

  /**
   * @given publish cid, tipset key, proposal
   * @when try to get current deal info
   * @then success
   */
  TEST_F(DealInfoManagerTest, Success) {
    CID publish_cid = "010001020001"_cid;

    DealProposal proposal;
    proposal.piece_cid = "010001020006"_cid;
    proposal.verified = false;
    proposal.client = Address::makeFromId(2);
    proposal.provider = Address::makeFromId(1);

    TipsetKey key{{CbCid::hash("02"_unhex)}};
    TipsetKey result_key{{CbCid::hash("03"_unhex), CbCid::hash("04"_unhex)}};

    DealId result_deal_id = 1;

    StorageDeal market_deal;
    market_deal.proposal = proposal;

    CurrentDealInfo result_deal{
        .deal_id = result_deal_id,
        .market_deal = market_deal,
        .publish_msg_tipset = result_key,
    };

    api_->StateSearchMsg =
        [&publish_cid, &result_deal_id, &result_key](
            const CID &cid) -> outcome::result<boost::optional<MsgWait>> {
      if (cid == publish_cid) {
        MsgWait lookup;
        lookup.receipt.exit_code = VMExitCode::kOk;
        lookup.tipset = result_key;
        PublishStorageDeals::Result result{
            .deals = {result_deal_id},
        };
        OUTCOME_TRYA(lookup.receipt.return_value, codec::cbor::encode(result));
        return lookup;
      }
      return ERROR_TEXT("ERROR");
    };

    api_->ChainGetMessage =
        [&publish_cid,
         &proposal](const CID &cid) -> outcome::result<UnsignedMessage> {
      if (cid == publish_cid) {
        UnsignedMessage result;
        PublishStorageDeals::Params params{.deals = {ClientDealProposal{
                                               .proposal = proposal,
                                           }}};
        OUTCOME_TRYA(result.params, codec::cbor::encode(params));
        return result;
      }
      return ERROR_TEXT("ERROR");
    };

    api_->StateMarketStorageDeal =
        [&result_deal_id, &key, &market_deal](
            DealId deal_id,
            const TipsetKey &tipset_key) -> outcome::result<StorageDeal> {
      if (deal_id == result_deal_id and tipset_key == key) {
        return market_deal;
      }
      return ERROR_TEXT("ERROR");
    };

    api_->StateLookupID =
        [&key](const Address &address,
               const TipsetKey &tipset_key) -> outcome::result<Address> {
      if (tipset_key == key) {
        return address;
      }
      return ERROR_TEXT("ERROR");
    };

    EXPECT_OUTCOME_EQ(manager_->getCurrentDealInfo(key, proposal, publish_cid),
                      result_deal);
  }

}  // namespace fc::mining
