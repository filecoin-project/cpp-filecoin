/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "miner/storage_fsm/impl/deal_info_manager_impl.hpp"

#include <gtest/gtest.h>
#include <vm/message/message.hpp>

#include "testutil/literals.hpp"
#include "testutil/mocks/api.hpp"
#include "testutil/outcome.hpp"
#include "vm/actor/builtin/v0/market/market_actor.hpp"
#include "vm/exit_code/exit_code.hpp"

namespace fc::mining {
  using api::Address;
  using api::MsgWait;
  using testing::_;
  using vm::VMExitCode;
  using vm::actor::builtin::types::market::ClientDealProposal;
  using vm::actor::builtin::v0::market::PublishStorageDeals;
  using vm::message::UnsignedMessage;

  template <typename F>
  auto mockSearch(F f) {
    return testing::Invoke(api::waitCb<boost::optional<MsgWait>>(
        [f](auto, auto, auto, auto, auto cb) { cb(f()); }));
  }

  class DealInfoManagerTest : public testing::Test {
   protected:
    virtual void SetUp() {
      manager_ = std::make_shared<DealInfoManagerImpl>(api_);
    }

    std::shared_ptr<FullNodeApi> api_{std::make_shared<FullNodeApi>()};
    std::shared_ptr<DealInfoManager> manager_;
    MOCK_API(api_, StateSearchMsg);

    CID publish_cid{"010001020001"_cid};
    TipsetKey key{{CbCid::hash("02"_unhex)}};
    TipsetKey result_key{{CbCid::hash("03"_unhex), CbCid::hash("04"_unhex)}};
    DealId result_deal_id{1};
  };

  /**
   * @given publish cid
   * @when try to get current deal info, but msg search is not ok
   * @then DealInfoManagerError::kNotOkExitCode occurs
   */
  TEST_F(DealInfoManagerTest, NonOkCode) {
    EXPECT_CALL(mock_StateSearchMsg, Call(_, publish_cid, _, _))
        .WillRepeatedly(mockSearch([] {
          MsgWait lookup;
          lookup.receipt.exit_code = VMExitCode::kFatal;
          return lookup;
        }));

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
    EXPECT_CALL(mock_StateSearchMsg, Call(_, publish_cid, _, _))
        .WillRepeatedly(mockSearch([] {
          MsgWait lookup;
          lookup.receipt.exit_code = VMExitCode::kOk;
          PublishStorageDeals::Result result{
              .deals = {1, 2},
          };
          lookup.receipt.return_value = codec::cbor::encode(result).value();
          return lookup;
        }));

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
    StorageDeal market_deal;

    CurrentDealInfo result_deal{
        .deal_id = result_deal_id,
        .market_deal = market_deal,
        .publish_msg_tipset = result_key,
    };

    EXPECT_CALL(mock_StateSearchMsg, Call(_, publish_cid, _, _))
        .WillRepeatedly(mockSearch([&] {
          MsgWait lookup;
          lookup.receipt.exit_code = VMExitCode::kOk;
          lookup.tipset = result_key;
          PublishStorageDeals::Result result{
              .deals = {result_deal_id},
          };
          lookup.receipt.return_value = codec::cbor::encode(result).value();
          return lookup;
        }));

    api_->StateMarketStorageDeal =
        [&](DealId deal_id,
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
    DealProposal proposal;
    proposal.verified = false;
    proposal.client = Address::makeFromId(2);
    proposal.provider = Address::makeFromId(1);

    StorageDeal market_deal;

    CurrentDealInfo result_deal{
        .deal_id = result_deal_id,
        .market_deal = market_deal,
        .publish_msg_tipset = result_key,
    };

    EXPECT_CALL(mock_StateSearchMsg, Call(_, publish_cid, _, _))
        .WillRepeatedly(mockSearch([&] {
          MsgWait lookup;
          lookup.receipt.exit_code = VMExitCode::kOk;
          lookup.tipset = result_key;
          PublishStorageDeals::Result result{
              .deals = {result_deal_id},
          };
          lookup.receipt.return_value = codec::cbor::encode(result).value();
          return lookup;
        }));

    Address another_provider = Address::makeFromId(2);

    api_->ChainGetMessage =
        [&](const CID &cid) -> outcome::result<UnsignedMessage> {
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
        [&](DealId deal_id,
            const TipsetKey &tipset_key) -> outcome::result<StorageDeal> {
      if (deal_id == result_deal_id and tipset_key == key) {
        return market_deal;
      }
      return ERROR_TEXT("ERROR");
    };

    api_->StateLookupID =
        [&](const Address &address,
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
    DealProposal proposal;
    proposal.piece_cid = "010001020006"_cid;
    proposal.verified = false;
    proposal.client = Address::makeFromId(2);
    proposal.provider = Address::makeFromId(1);

    StorageDeal market_deal;

    CurrentDealInfo result_deal{
        .deal_id = result_deal_id,
        .market_deal = market_deal,
        .publish_msg_tipset = result_key,
    };

    EXPECT_CALL(mock_StateSearchMsg, Call(_, publish_cid, _, _))
        .WillRepeatedly(mockSearch([&] {
          MsgWait lookup;
          lookup.receipt.exit_code = VMExitCode::kOk;
          lookup.tipset = result_key;
          PublishStorageDeals::Result result{
              .deals = {result_deal_id},
          };
          lookup.receipt.return_value = codec::cbor::encode(result).value();
          return lookup;
        }));

    Address another_provider = Address::makeFromId(2);

    api_->ChainGetMessage =
        [&, result_proposal{proposal}](
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
        [&](DealId deal_id,
            const TipsetKey &tipset_key) -> outcome::result<StorageDeal> {
      if (deal_id == result_deal_id and tipset_key == key) {
        return market_deal;
      }
      return ERROR_TEXT("ERROR");
    };

    api_->StateLookupID =
        [&](const Address &address,
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
    DealProposal proposal;
    proposal.piece_cid = "010001020006"_cid;
    proposal.verified = false;
    proposal.client = Address::makeFromId(2);
    proposal.provider = Address::makeFromId(1);

    StorageDeal market_deal;

    CurrentDealInfo result_deal{
        .deal_id = result_deal_id,
        .market_deal = market_deal,
        .publish_msg_tipset = result_key,
    };

    EXPECT_CALL(mock_StateSearchMsg, Call(_, publish_cid, _, _))
        .WillRepeatedly(mockSearch([&] {
          MsgWait lookup;
          lookup.receipt.exit_code = VMExitCode::kOk;
          lookup.tipset = result_key;
          PublishStorageDeals::Result result{
              .deals = {result_deal_id},
          };
          lookup.receipt.return_value = codec::cbor::encode(result).value();
          return lookup;
        }));

    api_->ChainGetMessage =
        [&](const CID &cid) -> outcome::result<UnsignedMessage> {
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
        [&](DealId deal_id,
            const TipsetKey &tipset_key) -> outcome::result<StorageDeal> {
      if (deal_id == result_deal_id and tipset_key == key) {
        return market_deal;
      }
      return ERROR_TEXT("ERROR");
    };

    api_->StateLookupID =
        [&](const Address &address,
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
    DealProposal proposal;
    proposal.piece_cid = "010001020006"_cid;
    proposal.verified = false;
    proposal.client = Address::makeFromId(2);
    proposal.provider = Address::makeFromId(1);

    StorageDeal market_deal;
    market_deal.proposal = proposal;

    CurrentDealInfo result_deal{
        .deal_id = result_deal_id,
        .market_deal = market_deal,
        .publish_msg_tipset = result_key,
    };

    EXPECT_CALL(mock_StateSearchMsg, Call(_, publish_cid, _, _))
        .WillRepeatedly(mockSearch([&] {
          MsgWait lookup;
          lookup.receipt.exit_code = VMExitCode::kOk;
          lookup.tipset = result_key;
          PublishStorageDeals::Result result{
              .deals = {result_deal_id},
          };
          lookup.receipt.return_value = codec::cbor::encode(result).value();
          return lookup;
        }));

    api_->ChainGetMessage =
        [&](const CID &cid) -> outcome::result<UnsignedMessage> {
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
        [&](DealId deal_id,
            const TipsetKey &tipset_key) -> outcome::result<StorageDeal> {
      if (deal_id == result_deal_id and tipset_key == key) {
        return market_deal;
      }
      return ERROR_TEXT("ERROR");
    };

    api_->StateLookupID =
        [&](const Address &address,
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
