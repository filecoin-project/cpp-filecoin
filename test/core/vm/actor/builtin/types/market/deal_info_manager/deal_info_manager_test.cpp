/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/types/market/deal_info_manager/impl/deal_info_manager_impl.hpp"

#include <gtest/gtest.h>

#include "testutil/literals.hpp"
#include "testutil/mocks/api.hpp"
#include "testutil/outcome.hpp"
#include "vm/actor/builtin/types/market/deal_proposal.hpp"
#include "vm/actor/builtin/v0/market/market_actor.hpp"
#include "vm/exit_code/exit_code.hpp"
#include "vm/message/message.hpp"

namespace fc::vm::actor::builtin::types::market::deal_info_manager {
  using api::Address;
  using api::MsgWait;
  using fc::vm::actor::builtin::types::Universal;
  using fc::vm::actor::builtin::types::market::DealProposal;
  using message::UnsignedMessage;
  using testing::_;
  using v0::market::PublishStorageDeals;

  template <typename F>
  auto mockSearch(F f) {
    return testing::Invoke([f](auto, auto, auto, auto) { return f(); });
  }

  class DealInfoManagerTest : public testing::Test {
   protected:
    virtual void SetUp() {
      manager_ = std::make_shared<DealInfoManagerImpl>(api_);
      api_->StateNetworkVersion = [](auto &tipset_key) {
        return vm::version::NetworkVersion::kVersion0;
      };
    }

    std::shared_ptr<FullNodeApi> api_{std::make_shared<FullNodeApi>()};
    std::shared_ptr<DealInfoManager> manager_;
    MOCK_API(api_, StateSearchMsg);

    CID publish_cid{"010001020001"_cid};
    TipsetKey result_key{{CbCid::hash("03"_unhex), CbCid::hash("04"_unhex)}};
    DealId result_deal_id{1};
  };

  /**
   * @given publish cid
   * @when try to get current deal info, but msg search is not ok
   * @then DealInfoManagerError::kNotOkExitCode occurs
   */
  TEST_F(DealInfoManagerTest, NonOkCode) {
    auto proposal = Universal<DealProposal>(ActorVersion::kVersion0);

    EXPECT_CALL(mock_StateSearchMsg, Call(_, publish_cid, _, _))
        .WillRepeatedly(mockSearch([&] {
          MsgWait lookup;
          lookup.message = publish_cid;
          lookup.receipt.exit_code = VMExitCode::kFatal;
          return lookup;
        }));

    EXPECT_OUTCOME_ERROR(DealInfoManagerError::kNotOkExitCode,
                         manager_->getCurrentDealInfo(proposal, publish_cid));
  }

  /**
   * @given publish cid, tipset key, proposal
   * @when try to get current deal info, but not found deal
   * @then DealInfoManagerError::kNotFound occurs
   */
  TEST_F(DealInfoManagerTest, NotFoundDeal) {
    auto proposal = Universal<DealProposal>(ActorVersion::kVersion0);
    proposal->verified = false;
    proposal->client = Address::makeFromId(2);
    proposal->provider = Address::makeFromId(1);

    StorageDeal market_deal;

    CurrentDealInfo result_deal{
        .deal_id = result_deal_id,
        .market_deal = market_deal,
        .publish_msg_tipset = result_key,
    };

    EXPECT_CALL(mock_StateSearchMsg, Call(_, publish_cid, _, _))
        .WillRepeatedly(mockSearch([&] {
          MsgWait lookup;
          lookup.message = publish_cid;
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
        auto proposal = Universal<DealProposal>(ActorVersion::kVersion0);
        proposal->piece_cid = "010001020005"_cid;
        proposal->verified = false;
        proposal->client = Address::makeFromId(2);
        proposal->provider = another_provider;
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
      if (deal_id == result_deal_id and tipset_key == result_key) {
        return market_deal;
      }
      return ERROR_TEXT("ERROR");
    };

    api_->StateLookupID =
        [&](const Address &address,
            const TipsetKey &tipset_key) -> outcome::result<Address> {
      if (tipset_key == result_key) {
        return address;
      }
      return ERROR_TEXT("ERROR");
    };

    EXPECT_OUTCOME_ERROR(DealInfoManagerError::kNotFound,
                         manager_->getCurrentDealInfo(proposal, publish_cid));
  }

  /**
   * @given publish cid, tipset key, proposal
   * @when try to get current deal info, but index more than deals
   * @then DealInfoManagerError::kOutOfRange occurs
   */
  TEST_F(DealInfoManagerTest, OutOfRangeDeal) {
    auto proposal = Universal<DealProposal>(ActorVersion::kVersion0);
    proposal->piece_cid = "010001020006"_cid;
    proposal->verified = false;
    proposal->client = Address::makeFromId(2);
    proposal->provider = Address::makeFromId(1);

    StorageDeal market_deal;

    CurrentDealInfo result_deal{
        .deal_id = result_deal_id,
        .market_deal = market_deal,
        .publish_msg_tipset = result_key,
    };

    EXPECT_CALL(mock_StateSearchMsg, Call(_, publish_cid, _, _))
        .WillRepeatedly(mockSearch([&] {
          MsgWait lookup;
          lookup.message = publish_cid;
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
        auto proposal = Universal<DealProposal>(ActorVersion::kVersion0);
        proposal->verified = false;
        proposal->client = Address::makeFromId(2);
        proposal->piece_cid = "010001020005"_cid;
        proposal->provider = another_provider;
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
      if (deal_id == result_deal_id and tipset_key == result_key) {
        return market_deal;
      }
      return ERROR_TEXT("ERROR");
    };

    api_->StateLookupID =
        [&](const Address &address,
            const TipsetKey &tipset_key) -> outcome::result<Address> {
      if (tipset_key == result_key) {
        return address;
      }
      return ERROR_TEXT("ERROR");
    };

    const auto res = manager_->getCurrentDealInfo(proposal, publish_cid);
    EXPECT_TRUE(res.has_error());
    EXPECT_EQ(res.error().message(),
              "publishDealsResult: deal index out of bound");
  }

  /**
   * @given publish cid, tipset key, proposal
   * @when try to get current deal info, but market proposal is another
   * @then DealInfoManagerError::kDealProposalNotMatch occurs
   */
  TEST_F(DealInfoManagerTest, NotMatchProposal) {
    auto proposal = Universal<DealProposal>(ActorVersion::kVersion0);
    proposal->piece_cid = "010001020006"_cid;
    proposal->verified = false;
    proposal->client = Address::makeFromId(2);
    proposal->provider = Address::makeFromId(1);

    StorageDeal market_deal;
    market_deal.proposal = Universal<DealProposal>(ActorVersion::kVersion0);

    CurrentDealInfo result_deal{
        .deal_id = result_deal_id,
        .market_deal = market_deal,
        .publish_msg_tipset = result_key,
    };

    EXPECT_CALL(mock_StateSearchMsg, Call(_, publish_cid, _, _))
        .WillRepeatedly(mockSearch([&] {
          MsgWait lookup;
          lookup.message = publish_cid;
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
      if (deal_id == result_deal_id and tipset_key == result_key) {
        return market_deal;
      }
      return ERROR_TEXT("ERROR");
    };

    api_->StateLookupID =
        [&](const Address &address,
            const TipsetKey &tipset_key) -> outcome::result<Address> {
      if (tipset_key == result_key) {
        return address;
      }
      return ERROR_TEXT("ERROR");
    };

    EXPECT_OUTCOME_ERROR(DealInfoManagerError::kDealProposalNotMatch,
                         manager_->getCurrentDealInfo(proposal, publish_cid));
  }

  /**
   * @given publish cid, tipset key, proposal
   * @when try to get current deal info
   * @then success
   */
  TEST_F(DealInfoManagerTest, Success) {
    auto proposal = Universal<DealProposal>(ActorVersion::kVersion0);
    proposal->piece_cid = "010001020006"_cid;
    proposal->verified = false;
    proposal->client = Address::makeFromId(2);
    proposal->provider = Address::makeFromId(1);

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
          lookup.message = publish_cid;
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
      if (deal_id == result_deal_id and tipset_key == result_key) {
        return market_deal;
      }
      return ERROR_TEXT("ERROR");
    };

    api_->StateLookupID =
        [&](const Address &address,
            const TipsetKey &tipset_key) -> outcome::result<Address> {
      if (tipset_key == result_key) {
        return address;
      }
      return ERROR_TEXT("ERROR");
    };

    EXPECT_OUTCOME_EQ(manager_->getCurrentDealInfo(proposal, publish_cid),
                      result_deal);
  }

}  // namespace fc::vm::actor::builtin::types::market::deal_info_manager
