/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "miner/storage_fsm/impl/checks.hpp"

#include <gtest/gtest.h>

#include "storage/ipfs/impl/in_memory_datastore.hpp"
#include "testutil/literals.hpp"
#include "testutil/mocks/api.hpp"
#include "testutil/mocks/proofs/proof_engine_mock.hpp"
#include "testutil/outcome.hpp"
#include "testutil/vm/actor/builtin/actor_test_util.hpp"
#include "vm/actor/actor.hpp"
#include "vm/actor/builtin/types/miner/sector_info.hpp"
#include "vm/actor/builtin/v5/market/market_actor.hpp"
#include "vm/actor/codes.hpp"
#include "vm/exit_code/exit_code.hpp"

namespace fc::mining::checks {
  using api::DomainSeparationTag;
  using api::InvocResult;
  using api::MinerInfo;
  using api::Randomness;
  using api::SectorNumber;
  using api::SectorOnChainInfo;
  using api::UnsignedMessage;
  using primitives::ActorId;
  using primitives::tipset::Tipset;
  using primitives::tipset::TipsetCPtr;
  using storage::ipfs::InMemoryDatastore;
  using storage::ipfs::IpfsDatastore;
  using testing::_;
  using types::DealInfo;
  using types::DealSchedule;
  using types::PaddedPieceSize;
  using types::Piece;
  using types::PieceInfo;
  using vm::VMExitCode;
  using vm::actor::Actor;
  using vm::actor::builtin::makeMinerActorState;
  using vm::actor::builtin::states::MinerActorStatePtr;
  using vm::actor::builtin::types::miner::kPreCommitChallengeDelay;
  using vm::actor::builtin::v5::market::ComputeDataCommitment;
  using vm::runtime::MockRuntime;

  class CheckPieces : public testing::Test {
   protected:
    void SetUp() override {
      miner_id_ = 42;
      miner_addr_ = Address::makeFromId(miner_id_);

      api_ = std::make_shared<FullNodeApi>();
    }

    ActorId miner_id_;
    Address miner_addr_;
    std::shared_ptr<FullNodeApi> api_;
  };

  /**
   * @given filler piece with wrong CID
   * @when try to check pieces
   * @then ChecksError::kInvalidDeal occurs
   */
  TEST_F(CheckPieces, FillPieceNotEqualCID) {
    std::shared_ptr<SectorInfo> info = std::make_shared<SectorInfo>();
    info->pieces = {
        Piece{
            .piece = PieceInfo{.size = PaddedPieceSize(2048),
                               .cid = "010001020001"_cid},
            .deal_info = boost::none,
        },
    };

    api_->ChainHead = []() -> outcome::result<TipsetCPtr> {
      return outcome::success();
    };

    EXPECT_OUTCOME_ERROR(ChecksError::kInvalidDeal,
                         checkPieces(miner_addr_, info, api_));
  }

  /**
   * @given filler piece with correct CID
   * @when try to check pieces
   * @then success
   */
  TEST_F(CheckPieces, FillPieceEqualCID) {
    std::shared_ptr<SectorInfo> info = std::make_shared<SectorInfo>();
    EXPECT_OUTCOME_TRUE(
        cid,
        CID::fromString("baga6ea4seaqpy7usqklokfx2vxuynmupslkeutzexe2uqurdg5vht"
                        "ebhxqmpqmy"));  // from lotus
    info->pieces = {
        Piece{
            .piece = PieceInfo{.size = PaddedPieceSize(2048), .cid = cid},
            .deal_info = boost::none,
        },
    };

    api_->ChainHead = []() -> outcome::result<TipsetCPtr> {
      return outcome::success();
    };

    EXPECT_OUTCOME_TRUE_1(checkPieces(miner_addr_, info, api_));
  }

  /**
   * @given piece, api, deal
   * @when try to check pieces, but deal proposal has wrong provider
   * @then ChecksError::kInvalidDeal occurs
   */
  TEST_F(CheckPieces, WrongProvider) {
    std::shared_ptr<SectorInfo> info = std::make_shared<SectorInfo>();
    api::DealId deal_id = 1;
    info->pieces = {
        Piece{
            .piece = PieceInfo{.size = PaddedPieceSize(2048),
                               .cid = "010001020001"_cid},
            .deal_info =
                DealInfo{
                    .publish_cid = "010001020002"_cid,
                    .deal_id = deal_id,
                    .deal_proposal = boost::none,
                    .deal_schedule =
                        DealSchedule{
                            .start_epoch = 1,
                            .end_epoch = 1,
                        },
                    .is_keep_unsealed = false,
                },
        },
    };

    TipsetKey head_key;
    api_->ChainHead = [&head_key]() -> outcome::result<TipsetCPtr> {
      auto tip =
          std::make_shared<Tipset>(head_key, std::vector<api::BlockHeader>());
      return tip;
    };

    api_->StateMarketStorageDeal =
        [id{miner_id_}, deal_id, &head_key](
            api::DealId did,
            const TipsetKey &key) -> outcome::result<api::StorageDeal> {
      if (did == deal_id and key == head_key) {
        api::StorageDeal res;
        res.proposal.provider = Address::makeFromId(id + 1);
        return res;
      }

      return ERROR_TEXT("ERROR");
    };

    EXPECT_OUTCOME_ERROR(ChecksError::kInvalidDeal,
                         checkPieces(miner_addr_, info, api_));
  }

  /**
   * @given piece, api, deal
   * @when try to check pieces, but deal proposal has wrong piece cid
   * @then ChecksError::kInvalidDeal occurs
   */
  TEST_F(CheckPieces, WrongPieceCid) {
    std::shared_ptr<SectorInfo> info = std::make_shared<SectorInfo>();
    api::DealId deal_id = 1;
    info->pieces = {
        Piece{
            .piece = PieceInfo{.size = PaddedPieceSize(2048),
                               .cid = "010001020001"_cid},
            .deal_info =
                DealInfo{
                    .publish_cid = "010001020002"_cid,
                    .deal_id = deal_id,
                    .deal_proposal = boost::none,
                    .deal_schedule =
                        DealSchedule{
                            .start_epoch = 1,
                            .end_epoch = 1,
                        },
                    .is_keep_unsealed = false,
                },
        },
    };

    TipsetKey head_key;
    api_->ChainHead = [&head_key]() -> outcome::result<TipsetCPtr> {
      auto tip =
          std::make_shared<Tipset>(head_key, std::vector<api::BlockHeader>());
      return tip;
    };

    api_->StateMarketStorageDeal =
        [id{miner_id_}, deal_id, &head_key](
            api::DealId did,
            const TipsetKey &key) -> outcome::result<api::StorageDeal> {
      if (did == deal_id and key == head_key) {
        api::StorageDeal res;
        res.proposal.provider = Address::makeFromId(id);
        res.proposal.piece_cid = "010001020002"_cid;
        return res;
      }

      return ERROR_TEXT("ERROR");
    };

    EXPECT_OUTCOME_ERROR(ChecksError::kInvalidDeal,
                         checkPieces(miner_addr_, info, api_));
  }

  /**
   * @given piece, api, deal
   * @when try to check pieces, but deal proposal has wrong piece size
   * @then ChecksError::kInvalidDeal occurs
   */
  TEST_F(CheckPieces, WrongPieceSize) {
    std::shared_ptr<SectorInfo> info = std::make_shared<SectorInfo>();
    api::DealId deal_id = 1;

    PieceInfo piece{.size = PaddedPieceSize(2048), .cid = "010001020001"_cid};
    info->pieces = {
        Piece{
            .piece = piece,
            .deal_info =
                DealInfo{
                    .publish_cid = "010001020002"_cid,
                    .deal_id = deal_id,
                    .deal_proposal = boost::none,
                    .deal_schedule =
                        DealSchedule{
                            .start_epoch = 1,
                            .end_epoch = 1,
                        },
                    .is_keep_unsealed = false,
                },
        },
    };

    TipsetKey head_key;
    api_->ChainHead = [&head_key]() -> outcome::result<TipsetCPtr> {
      auto tip =
          std::make_shared<Tipset>(head_key, std::vector<api::BlockHeader>());
      return tip;
    };

    api_->StateMarketStorageDeal =
        [&piece, id{miner_id_}, deal_id, &head_key](
            api::DealId did,
            const TipsetKey &key) -> outcome::result<api::StorageDeal> {
      if (did == deal_id and key == head_key) {
        api::StorageDeal res;
        res.proposal.provider = Address::makeFromId(id);
        res.proposal.piece_cid = piece.cid;
        res.proposal.piece_size = piece.size + 1;
        return res;
      }

      return ERROR_TEXT("ERROR");
    };

    EXPECT_OUTCOME_ERROR(ChecksError::kInvalidDeal,
                         checkPieces(miner_addr_, info, api_));
  }

  /**
   * @given piece, api, deal
   * @when try to check pieces, but deal proposal has expired start epoch
   * @then ChecksError::kExpiredDeal occurs
   */
  TEST_F(CheckPieces, ExpiredDeal) {
    std::shared_ptr<SectorInfo> info = std::make_shared<SectorInfo>();
    api::DealId deal_id = 1;

    PieceInfo piece{.size = PaddedPieceSize(2048), .cid = "010001020001"_cid};
    info->pieces = {
        Piece{
            .piece = piece,
            .deal_info =
                DealInfo{
                    .publish_cid = "010001020002"_cid,
                    .deal_id = deal_id,
                    .deal_proposal = boost::none,
                    .deal_schedule =
                        DealSchedule{
                            .start_epoch = 1,
                            .end_epoch = 1,
                        },
                    .is_keep_unsealed = false,
                },
        },
    };

    TipsetKey head_key;
    api_->ChainHead = [&head_key]() -> outcome::result<TipsetCPtr> {
      auto tip =
          std::make_shared<Tipset>(head_key, std::vector<api::BlockHeader>());
      return tip;
    };

    api_->StateMarketStorageDeal =
        [&piece, id{miner_id_}, deal_id, &head_key](
            api::DealId did,
            const TipsetKey &key) -> outcome::result<api::StorageDeal> {
      if (did == deal_id and key == head_key) {
        api::StorageDeal res;
        res.proposal.provider = Address::makeFromId(id);
        res.proposal.piece_cid = piece.cid;
        res.proposal.piece_size = piece.size;
        res.proposal.start_epoch = 0;
        return res;
      }

      return ERROR_TEXT("ERROR");
    };

    EXPECT_OUTCOME_ERROR(ChecksError::kExpiredDeal,
                         checkPieces(miner_addr_, info, api_));
  }

  MATCHER_P(methodMatcher, method, "compare msg method name") {
    return (arg.method == method);
  }

  class CheckPrecommit : public testing::Test {
   protected:
    void SetUp() override {
      miner_id_ = 42;
      miner_addr_ = Address::makeFromId(miner_id_);
      ipld_ = std::make_shared<InMemoryDatastore>();
      version_ = api::NetworkVersion::kVersion13;
      const auto actor_version = actorVersion(version_);
      ipld_->actor_version = actor_version;
      actor_state_ = makeMinerActorState(ipld_, actor_version);
    }

    NetworkVersion version_;
    std::shared_ptr<IpfsDatastore> ipld_;
    MinerActorStatePtr actor_state_;
    ActorId miner_id_;
    Address miner_addr_;
    std::shared_ptr<FullNodeApi> api_ = std::make_shared<FullNodeApi>();
  };

  /**
   * @given info, valid pieces,
   * @when try to check precommit, but commd is not equal
   * @then ChecksError::kBadCommD occurs
   */
  TEST_F(CheckPrecommit, BadCommD) {
    std::shared_ptr<SectorInfo> info = std::make_shared<SectorInfo>();
    api::DealId deal_id = 1;

    PieceInfo piece{.size = PaddedPieceSize(2048), .cid = "010001020001"_cid};
    info->comm_d = "010001020001"_cid;
    info->pieces = {
        Piece{
            .piece = piece,
            .deal_info =
                DealInfo{
                    .publish_cid = "010001020002"_cid,
                    .deal_id = deal_id,
                    .deal_proposal = boost::none,
                    .deal_schedule =
                        DealSchedule{
                            .start_epoch = 1,
                            .end_epoch = 1,
                        },
                    .is_keep_unsealed = false,
                },
        },
    };

    MOCK_API(api_, ChainHead);
    TipsetKey head_key;
    auto head_tipset =
        std::make_shared<Tipset>(head_key, std::vector<api::BlockHeader>());
    EXPECT_CALL(mock_ChainHead, Call())
        .WillOnce(testing::Return(outcome::success(head_tipset)));

    MOCK_API(api_, StateMarketStorageDeal);
    api::StorageDeal deal;
    deal.proposal.provider = Address::makeFromId(miner_id_);
    deal.proposal.piece_cid = piece.cid;
    deal.proposal.piece_size = piece.size;
    deal.proposal.start_epoch = 1;
    EXPECT_CALL(mock_StateMarketStorageDeal, Call(deal_id, head_key))
        .WillOnce(testing::Return(outcome::success(deal)));

    TipsetKey precommit_key{{CbCid::hash("01"_unhex),
                             CbCid::hash("02"_unhex),
                             CbCid::hash("03"_unhex)}};
    MOCK_API(api_, StateCall);
    InvocResult invoc_result;
    invoc_result.receipt.exit_code = VMExitCode::kOk;
    ComputeDataCommitment::Result result{.commds = {"010001020002"_cid}};
    EXPECT_OUTCOME_TRUE(cid_buf, codec::cbor::encode(result));
    invoc_result.receipt.return_value = cid_buf;
    EXPECT_CALL(
        mock_StateCall,
        Call(methodMatcher(ComputeDataCommitment::Number), precommit_key))
        .WillOnce(testing::Return(outcome::success(invoc_result)));

    EXPECT_OUTCOME_ERROR(
        ChecksError::kBadCommD,
        checkPrecommit(miner_addr_, info, precommit_key, 0, api_));
  }

  /**
   * @given info, valid pieces, expired ticket
   * @when try to check precommit
   * @then ChecksError::kExpiredTicket occurs
   */
  TEST_F(CheckPrecommit, ExpiredTicket) {
    std::shared_ptr<SectorInfo> info = std::make_shared<SectorInfo>();
    api::DealId deal_id = 1;

    PieceInfo piece{.size = PaddedPieceSize(2048), .cid = "010001020001"_cid};
    info->comm_d = "010001020001"_cid;
    info->pieces = {
        Piece{
            .piece = piece,
            .deal_info =
                DealInfo{
                    .publish_cid = "010001020002"_cid,
                    .deal_id = deal_id,
                    .deal_proposal = boost::none,
                    .deal_schedule =
                        DealSchedule{
                            .start_epoch = 1,
                            .end_epoch = 1,
                        },
                    .is_keep_unsealed = false,
                },
        },
    };

    MOCK_API(api_, ChainHead);
    TipsetKey head_key;
    auto head_tipset =
        std::make_shared<Tipset>(head_key, std::vector<api::BlockHeader>());
    EXPECT_CALL(mock_ChainHead, Call())
        .WillOnce(testing::Return(outcome::success(head_tipset)));

    MOCK_API(api_, StateMarketStorageDeal);
    api::StorageDeal deal;
    deal.proposal.provider = Address::makeFromId(miner_id_);
    deal.proposal.piece_cid = piece.cid;
    deal.proposal.piece_size = piece.size;
    deal.proposal.start_epoch = 1;
    EXPECT_CALL(mock_StateMarketStorageDeal, Call(deal_id, head_key))
        .WillOnce(testing::Return(outcome::success(deal)));

    TipsetKey precommit_key{{CbCid::hash("01"_unhex),
                             CbCid::hash("02"_unhex),
                             CbCid::hash("03"_unhex)}};
    MOCK_API(api_, StateCall);
    InvocResult invoc_result;
    invoc_result.receipt.exit_code = VMExitCode::kOk;
    ComputeDataCommitment::Result result{.commds = {*info->comm_d}};
    EXPECT_OUTCOME_TRUE(cid_buf, codec::cbor::encode(result));
    invoc_result.receipt.return_value = cid_buf;
    EXPECT_CALL(
        mock_StateCall,
        Call(methodMatcher(ComputeDataCommitment::Number), precommit_key))
        .WillOnce(testing::Return(outcome::success(invoc_result)));

    MOCK_API(api_, StateNetworkVersion);
    EXPECT_CALL(mock_StateNetworkVersion, Call(precommit_key))
        .WillOnce(testing::Return(outcome::success(version_)));
    EXPECT_OUTCOME_TRUE(duration,
                        checks::getMaxProveCommitDuration(version_, info));
    ChainEpoch height = duration
                        + vm::actor::builtin::types::miner::kChainFinality
                        + info->ticket_epoch + 1;
    EXPECT_OUTCOME_ERROR(
        ChecksError::kExpiredTicket,
        checkPrecommit(miner_addr_, info, precommit_key, height, api_));
  }

  /**
   * @given info, valid pieces, ticket, precommit on chain
   * @when try to check precommit
   * @then ChecksError::kPrecommitOnChain occurs
   */
  TEST_F(CheckPrecommit, PrecommitOnChain) {
    std::shared_ptr<SectorInfo> info = std::make_shared<SectorInfo>();
    api::DealId deal_id = 1;

    SectorNumber sector = 1;
    PieceInfo piece{.size = PaddedPieceSize(2048), .cid = "010001020001"_cid};
    info->comm_d = "010001020001"_cid;
    info->sector_number = sector;
    info->pieces = {
        Piece{
            .piece = piece,
            .deal_info =
                DealInfo{
                    .publish_cid = "010001020002"_cid,
                    .deal_id = deal_id,
                    .deal_proposal = boost::none,
                    .deal_schedule =
                        DealSchedule{
                            .start_epoch = 1,
                            .end_epoch = 1,
                        },
                    .is_keep_unsealed = false,
                },
        },
    };

    TipsetKey head_key;
    api_->ChainHead = [&head_key]() -> outcome::result<TipsetCPtr> {
      auto tip =
          std::make_shared<Tipset>(head_key, std::vector<api::BlockHeader>());
      return tip;
    };

    api_->StateMarketStorageDeal =
        [&piece, id{miner_id_}, deal_id, &head_key](
            api::DealId did,
            const TipsetKey &key) -> outcome::result<api::StorageDeal> {
      if (did == deal_id and key == head_key) {
        api::StorageDeal res;
        res.proposal.provider = Address::makeFromId(id);
        res.proposal.piece_cid = piece.cid;
        res.proposal.piece_size = piece.size;
        res.proposal.start_epoch = 1;
        return res;
      }

      return ERROR_TEXT("ERROR");
    };

    TipsetKey precommit_key{{CbCid::hash("01"_unhex),
                             CbCid::hash("02"_unhex),
                             CbCid::hash("03"_unhex)}};
    MOCK_API(api_, StateCall);
    InvocResult invoc_result;
    invoc_result.receipt.exit_code = VMExitCode::kOk;
    ComputeDataCommitment::Result result{.commds = {*info->comm_d}};
    EXPECT_OUTCOME_TRUE(cid_buf, codec::cbor::encode(result));
    invoc_result.receipt.return_value = cid_buf;
    EXPECT_CALL(
        mock_StateCall,
        Call(methodMatcher(ComputeDataCommitment::Number), precommit_key))
        .WillOnce(testing::Return(outcome::success(invoc_result)));

    MOCK_API(api_, StateNetworkVersion);
    EXPECT_CALL(mock_StateNetworkVersion, Call(precommit_key))
        .Times(2)
        .WillRepeatedly(testing::Return(outcome::success(version_)));
    EXPECT_OUTCOME_TRUE(duration,
                        checks::getMaxProveCommitDuration(version_, info));
    ChainEpoch height = duration
                        + vm::actor::builtin::types::miner::kChainFinality
                        + info->ticket_epoch;

    auto actor_key{"010001020003"_cid};
    SectorPreCommitOnChainInfo some_info;
    some_info.info.sealed_cid = "010001020006"_cid;
    EXPECT_OUTCOME_TRUE_1(
        actor_state_->precommitted_sectors.set(sector, some_info));
    EXPECT_OUTCOME_TRUE(cid_root,
                        actor_state_->precommitted_sectors.hamt.flush());

    api_->ChainReadObj = [&](CID key) -> outcome::result<Buffer> {
      if (key == actor_key) {
        return codec::cbor::encode(actor_state_);
      }
      if (key == cid_root) {
        EXPECT_OUTCOME_TRUE(root, getCbor<storage::hamt::Node>(ipld_, cid_root));
        return codec::cbor::encode(root);
      }
      if (key == actor_state_->allocated_sectors) {
        return codec::cbor::encode(primitives::RleBitset());
      }
      return ERROR_TEXT("ERROR");
    };

    Actor actor;
    actor.code = vm::actor::builtin::v0::kStorageMinerCodeId;
    actor.head = actor_key;
    api_->StateGetActor = [&](const Address &addr, const TipsetKey &tipset_key)
        -> outcome::result<Actor> { return actor; };

    EXPECT_OUTCOME_ERROR(
        ChecksError::kPrecommitOnChain,
        checkPrecommit(miner_addr_, info, precommit_key, height, api_));
  }

  /**
   * @given info, valid pieces, ticket, precommit on chain
   * @when try to check precommit, but ticket with another epoch
   * @then ChecksError::kBadTicketEpoch occurs
   */
  TEST_F(CheckPrecommit, BadTicketEpoch) {
    std::shared_ptr<SectorInfo> info = std::make_shared<SectorInfo>();
    api::DealId deal_id = 1;

    SectorNumber sector = 1;
    PieceInfo piece{.size = PaddedPieceSize(2048), .cid = "010001020001"_cid};
    info->comm_d = "010001020001"_cid;
    info->sector_number = sector;
    info->pieces = {
        Piece{
            .piece = piece,
            .deal_info =
                DealInfo{
                    .publish_cid = "010001020002"_cid,
                    .deal_id = deal_id,
                    .deal_proposal = boost::none,
                    .deal_schedule =
                        DealSchedule{
                            .start_epoch = 1,
                            .end_epoch = 1,
                        },
                    .is_keep_unsealed = false,
                },
        },
    };

    TipsetKey head_key;
    api_->ChainHead = [&head_key]() -> outcome::result<TipsetCPtr> {
      auto tip =
          std::make_shared<Tipset>(head_key, std::vector<api::BlockHeader>());
      return tip;
    };

    api_->StateMarketStorageDeal =
        [&piece, id{miner_id_}, deal_id, &head_key](
            api::DealId did,
            const TipsetKey &key) -> outcome::result<api::StorageDeal> {
      if (did == deal_id and key == head_key) {
        api::StorageDeal res;
        res.proposal.provider = Address::makeFromId(id);
        res.proposal.piece_cid = piece.cid;
        res.proposal.piece_size = piece.size;
        res.proposal.start_epoch = 1;
        return res;
      }

      return ERROR_TEXT("ERROR");
    };

    TipsetKey precommit_key{{CbCid::hash("01"_unhex),
                             CbCid::hash("02"_unhex),
                             CbCid::hash("03"_unhex)}};
    MOCK_API(api_, StateCall);
    InvocResult invoc_result;
    invoc_result.receipt.exit_code = VMExitCode::kOk;
    ComputeDataCommitment::Result result{.commds = {*info->comm_d}};
    EXPECT_OUTCOME_TRUE(cid_buf, codec::cbor::encode(result));
    invoc_result.receipt.return_value = cid_buf;
    EXPECT_CALL(
        mock_StateCall,
        Call(methodMatcher(ComputeDataCommitment::Number), precommit_key))
        .WillOnce(testing::Return(outcome::success(invoc_result)));

    MOCK_API(api_, StateNetworkVersion);
    EXPECT_CALL(mock_StateNetworkVersion, Call(precommit_key))
        .Times(2)
        .WillRepeatedly(testing::Return(outcome::success(version_)));
    EXPECT_OUTCOME_TRUE(duration,
                        checks::getMaxProveCommitDuration(version_, info));
    ChainEpoch height = duration
                        + vm::actor::builtin::types::miner::kChainFinality
                        + info->ticket_epoch;

    auto actor_key{"010001020003"_cid};

    SectorPreCommitOnChainInfo some_info;
    some_info.info.sealed_cid = "010001020006"_cid;
    some_info.info.seal_epoch = info->ticket_epoch + 1;
    EXPECT_OUTCOME_TRUE_1(
        actor_state_->precommitted_sectors.set(sector, some_info));
    EXPECT_OUTCOME_TRUE(cid_root,
                        actor_state_->precommitted_sectors.hamt.flush());

    api_->ChainReadObj = [&](CID key) -> outcome::result<Buffer> {
      if (key == actor_key) {
        return codec::cbor::encode(actor_state_);
      }
      if (key == cid_root) {
        EXPECT_OUTCOME_TRUE(root, getCbor<storage::hamt::Node>(ipld_, cid_root));
        return codec::cbor::encode(root);
      }
      if (key == actor_state_->allocated_sectors) {
        return codec::cbor::encode(primitives::RleBitset());
      }
      return ERROR_TEXT("ERROR");
    };

    Actor actor;
    actor.code = vm::actor::builtin::v0::kStorageMinerCodeId;
    actor.head = actor_key;
    api_->StateGetActor = [&](const Address &addr, const TipsetKey &tipset_key)
        -> outcome::result<Actor> { return actor; };

    EXPECT_OUTCOME_ERROR(
        ChecksError::kBadTicketEpoch,
        checkPrecommit(miner_addr_, info, precommit_key, height, api_));
  }

  class CheckCommit : public testing::Test {
   protected:
    void SetUp() override {
      miner_id_ = 42;
      miner_addr_ = Address::makeFromId(miner_id_);

      api_ = std::make_shared<FullNodeApi>();

      version_ = api::NetworkVersion::kVersion13;
      api_->StateNetworkVersion = [&](const TipsetKey &tipset_key)
          -> outcome::result<api::NetworkVersion> { return version_; };

      proofs_ = std::make_shared<proofs::ProofEngineMock>();
      ipld_ = std::make_shared<InMemoryDatastore>();
      const auto actor_version = actorVersion(version_);
      ipld_->actor_version = actor_version;
      actor_state_ = makeMinerActorState(ipld_, actor_version);
    }

    std::shared_ptr<IpfsDatastore> ipld_;
    MinerActorStatePtr actor_state_;
    api::NetworkVersion version_;
    ActorId miner_id_;
    Address miner_addr_;
    std::shared_ptr<FullNodeApi> api_;
    std::shared_ptr<proofs::ProofEngineMock> proofs_;
  };

  /**
   * @given sector info, proof, tipset_key
   * @when try to check commit, but info has 0 seed epoch
   * @then ChecksError::kBadSeed occurs
   */
  TEST_F(CheckCommit, BadSeedWithZeroEpoch) {
    std::shared_ptr<SectorInfo> info = std::make_shared<SectorInfo>();
    info->seed_epoch = 0;

    Proof proof{{1, 2, 3}};

    TipsetKey commit_key{{CbCid::hash("01"_unhex),
                          CbCid::hash("02"_unhex),
                          CbCid::hash("03"_unhex)}};
    EXPECT_OUTCOME_ERROR(
        ChecksError::kBadSeed,
        checkCommit(miner_addr_, info, proof, commit_key, api_, proofs_));
  }

  /**
   * @given sector info, proof, tipset_key
   * @when try to check commit, but precommit sector hasn't sector, but message
   * in info
   * @then ChecksError::kCommitWaitFail occurs
   */
  TEST_F(CheckCommit, CommitWaitFail) {
    std::shared_ptr<SectorInfo> info = std::make_shared<SectorInfo>();
    SectorNumber sector = 1;
    info->sector_number = sector;
    info->seed_epoch = 1;
    info->message = "010001020001"_cid;

    Proof proof{{1, 2, 3}};

    TipsetKey commit_key{{CbCid::hash("01"_unhex),
                          CbCid::hash("02"_unhex),
                          CbCid::hash("03"_unhex)}};

    auto actor_key{"010001020003"_cid};
    SectorPreCommitOnChainInfo some_info;
    some_info.info.sealed_cid = "010001020006"_cid;
    some_info.info.seal_epoch = info->ticket_epoch + 1;
    EXPECT_OUTCOME_TRUE_1(
        actor_state_->precommitted_sectors.set(sector + 1, some_info));
    EXPECT_OUTCOME_TRUE(cid_root,
                        actor_state_->precommitted_sectors.hamt.flush());

    api_->ChainReadObj = [&](CID key) -> outcome::result<Buffer> {
      if (key == actor_key) {
        return codec::cbor::encode(actor_state_);
      }
      if (key == cid_root) {
        EXPECT_OUTCOME_TRUE(root, getCbor<storage::hamt::Node>(ipld_, cid_root));
        return codec::cbor::encode(root);
      }
      if (key == actor_state_->allocated_sectors) {
        auto bitset = primitives::RleBitset();
        bitset.insert(sector);
        return codec::cbor::encode(bitset);
      }
      return ERROR_TEXT("ERROR");
    };

    Actor actor;
    actor.code = vm::actor::builtin::v0::kStorageMinerCodeId;
    actor.head = actor_key;
    api_->StateGetActor = [&](const Address &addr, const TipsetKey &tipset_key)
        -> outcome::result<Actor> { return actor; };

    EXPECT_OUTCOME_ERROR(
        ChecksError::kCommitWaitFail,
        checkCommit(miner_addr_, info, proof, commit_key, api_, proofs_));
  }

  /**
   * @given sector info, proof, tipset_key
   * @when try to check commit, but precommit not found
   * @then ChecksError::kPrecommitNotFound occurs
   */
  TEST_F(CheckCommit, PrecommitNotFound) {
    std::shared_ptr<SectorInfo> info = std::make_shared<SectorInfo>();
    SectorNumber sector = 1;
    info->sector_number = sector;
    info->seed_epoch = 1;
    info->message = "010001020001"_cid;

    Proof proof{{1, 2, 3}};

    TipsetKey commit_key{{CbCid::hash("01"_unhex),
                          CbCid::hash("02"_unhex),
                          CbCid::hash("03"_unhex)}};
    auto actor_key{"010001020003"_cid};
    SectorPreCommitOnChainInfo some_info;
    some_info.info.sealed_cid = "010001020006"_cid;
    some_info.info.seal_epoch = info->ticket_epoch + 1;
    EXPECT_OUTCOME_TRUE_1(
        actor_state_->precommitted_sectors.set(sector + 1, some_info));
    EXPECT_OUTCOME_TRUE(cid_root,
                        actor_state_->precommitted_sectors.hamt.flush());
    api_->ChainReadObj = [&](CID key) -> outcome::result<Buffer> {
      if (key == actor_key) {
        return codec::cbor::encode(actor_state_);
      }
      if (key == cid_root) {
        EXPECT_OUTCOME_TRUE(root, getCbor<storage::hamt::Node>(ipld_, cid_root));
        return codec::cbor::encode(root);
      }
      if (key == actor_state_->allocated_sectors) {
        return codec::cbor::encode(primitives::RleBitset());
      }
      return ERROR_TEXT("ERROR");
    };

    Actor actor;
    actor.code = vm::actor::builtin::v0::kStorageMinerCodeId;
    actor.head = actor_key;
    api_->StateGetActor = [&](const Address &addr, const TipsetKey &tipset_key)
        -> outcome::result<Actor> { return actor; };

    EXPECT_OUTCOME_ERROR(
        ChecksError::kPrecommitNotFound,
        checkCommit(miner_addr_, info, proof, commit_key, api_, proofs_));
  }

  /**
   * @given sector info, proof, tipset_key
   * @when try to check commit, but info has different seed epoch
   * @then ChecksError::kBadSeed occurs
   */
  TEST_F(CheckCommit, BadSeedWithPrecommitEpoch) {
    std::shared_ptr<SectorInfo> info = std::make_shared<SectorInfo>();
    SectorNumber sector = 1;
    info->sector_number = sector;
    info->seed_epoch = kPreCommitChallengeDelay;
    info->message = "010001020001"_cid;
    info->seed = Randomness{{1, 2, 3, 4, 5}};

    Proof proof{{1, 2, 3}};

    TipsetKey commit_key{{CbCid::hash("01"_unhex),
                          CbCid::hash("02"_unhex),
                          CbCid::hash("03"_unhex)}};
    auto actor_key{"010001020003"_cid};
    SectorPreCommitOnChainInfo some_info;
    some_info.info.sealed_cid = "010001020006"_cid;
    some_info.info.seal_epoch = info->ticket_epoch + 1;
    some_info.precommit_epoch = info->seed_epoch - kPreCommitChallengeDelay + 1;
    EXPECT_OUTCOME_TRUE_1(
        actor_state_->precommitted_sectors.set(sector, some_info));
    EXPECT_OUTCOME_TRUE(cid_root,
                        actor_state_->precommitted_sectors.hamt.flush());

    api_->ChainReadObj = [&](CID key) -> outcome::result<Buffer> {
      if (key == actor_key) {
        return codec::cbor::encode(actor_state_);
      }
      if (key == cid_root) {
        EXPECT_OUTCOME_TRUE(root, getCbor<storage::hamt::Node>(ipld_, cid_root));
        return codec::cbor::encode(root);
      }
      if (key == actor_state_->allocated_sectors) {
        return codec::cbor::encode(primitives::RleBitset());
      }
      return ERROR_TEXT("ERROR");
    };

    Actor actor;
    actor.code = vm::actor::builtin::v0::kStorageMinerCodeId;
    actor.head = actor_key;
    api_->StateGetActor = [&](const Address &addr, const TipsetKey &tipset_key)
        -> outcome::result<Actor> { return actor; };

    EXPECT_OUTCOME_ERROR(
        ChecksError::kBadSeed,
        checkCommit(miner_addr_, info, proof, commit_key, api_, proofs_));
  }

  /**
   * @given sector info, proof, tipset_key
   * @when try to check commit, but info has different seed from precommit
   * @then ChecksError::kBadSeed occurs
   */
  TEST_F(CheckCommit, BadSeedWithPrecommitDifferntSeed) {
    std::shared_ptr<SectorInfo> info = std::make_shared<SectorInfo>();
    SectorNumber sector = 1;
    info->sector_number = sector;
    info->seed_epoch = kPreCommitChallengeDelay;
    info->message = "010001020001"_cid;
    info->seed = Randomness{{1, 2, 3, 4, 5}};

    Proof proof{{1, 2, 3}};

    TipsetKey commit_key{{CbCid::hash("01"_unhex),
                          CbCid::hash("02"_unhex),
                          CbCid::hash("03"_unhex)}};
    auto actor_key{"010001020003"_cid};
    SectorPreCommitOnChainInfo some_info;
    some_info.info.sealed_cid = "010001020006"_cid;
    some_info.info.seal_epoch = info->ticket_epoch + 1;
    some_info.precommit_epoch = info->seed_epoch - kPreCommitChallengeDelay;
    EXPECT_OUTCOME_TRUE_1(
        actor_state_->precommitted_sectors.set(sector, some_info));
    EXPECT_OUTCOME_TRUE(cid_root,
                        actor_state_->precommitted_sectors.hamt.flush());

    api_->ChainReadObj = [&](CID key) -> outcome::result<Buffer> {
      if (key == actor_key) {
        return codec::cbor::encode(actor_state_);
      }
      if (key == cid_root) {
        EXPECT_OUTCOME_TRUE(root, getCbor<storage::hamt::Node>(ipld_, cid_root));
        return codec::cbor::encode(root);
      }
      if (key == actor_state_->allocated_sectors) {
        return codec::cbor::encode(primitives::RleBitset());
      }
      return ERROR_TEXT("ERROR");
    };

    Actor actor;
    actor.code = vm::actor::builtin::v0::kStorageMinerCodeId;
    actor.head = actor_key;
    api_->StateGetActor = [&](const Address &addr, const TipsetKey &tipset_key)
        -> outcome::result<Actor> { return actor; };

    api_->ChainGetRandomnessFromBeacon =
        [&](const TipsetKey &key,
            DomainSeparationTag tag,
            ChainEpoch epoch,
            const Buffer &buf) -> outcome::result<Randomness> {
      if (key == commit_key
          and tag == DomainSeparationTag::InteractiveSealChallengeSeed
          and epoch == info->seed_epoch) {
        auto new_seed = info->seed;
        new_seed[0] = 0;
        return new_seed;
      }

      return ERROR_TEXT("ERROR");
    };

    EXPECT_OUTCOME_ERROR(
        ChecksError::kBadSeed,
        checkCommit(miner_addr_, info, proof, commit_key, api_, proofs_));
  }

  /**
   * @given sector info, proof, tipset_key
   * @when try to check commit, but comm_r not equal to sealed cid from
   * precommit
   * @then ChecksError::kBadSealedCid occurs
   */
  TEST_F(CheckCommit, BadSealedCid) {
    std::shared_ptr<SectorInfo> info = std::make_shared<SectorInfo>();
    SectorNumber sector = 1;
    info->sector_number = sector;
    info->seed_epoch = kPreCommitChallengeDelay;
    info->message = "010001020001"_cid;
    info->comm_r = "010001020005"_cid;
    info->seed = Randomness{{1, 2, 3, 4, 5}};

    Proof proof{{1, 2, 3}};

    TipsetKey commit_key{{CbCid::hash("01"_unhex),
                          CbCid::hash("02"_unhex),
                          CbCid::hash("03"_unhex)}};
    auto actor_key{"010001020003"_cid};
    SectorPreCommitOnChainInfo some_info;
    some_info.info.sealed_cid = "010001020006"_cid;
    some_info.info.seal_epoch = info->ticket_epoch + 1;
    some_info.precommit_epoch = info->seed_epoch - kPreCommitChallengeDelay;
    EXPECT_OUTCOME_TRUE_1(
        actor_state_->precommitted_sectors.set(sector, some_info));
    EXPECT_OUTCOME_TRUE(cid_root,
                        actor_state_->precommitted_sectors.hamt.flush());

    api_->ChainReadObj = [&](CID key) -> outcome::result<Buffer> {
      if (key == actor_key) {
        return codec::cbor::encode(actor_state_);
      }
      if (key == cid_root) {
        EXPECT_OUTCOME_TRUE(root, getCbor<storage::hamt::Node>(ipld_, cid_root));
        return codec::cbor::encode(root);
      }
      if (key == actor_state_->allocated_sectors) {
        return codec::cbor::encode(primitives::RleBitset());
      }
      return ERROR_TEXT("ERROR");
    };

    Actor actor;
    actor.code = vm::actor::builtin::v0::kStorageMinerCodeId;
    actor.head = actor_key;
    api_->StateGetActor = [&](const Address &addr, const TipsetKey &tipset_key)
        -> outcome::result<Actor> { return actor; };

    api_->ChainGetRandomnessFromBeacon =
        [&](const TipsetKey &key,
            DomainSeparationTag tag,
            ChainEpoch epoch,
            const Buffer &buf) -> outcome::result<Randomness> {
      if (key == commit_key
          and tag == DomainSeparationTag::InteractiveSealChallengeSeed
          and epoch == info->seed_epoch) {
        return info->seed;
      }

      return ERROR_TEXT("ERROR");
    };

    EXPECT_OUTCOME_ERROR(
        ChecksError::kBadSealedCid,
        checkCommit(miner_addr_, info, proof, commit_key, api_, proofs_));
  }

  /**
   * @given sector info, proof, tipset_key
   * @when try to check commit, but seal is not correct
   * @then ChecksError::kInvalidProof occurs
   */
  TEST_F(CheckCommit, InvalidProof) {
    std::shared_ptr<SectorInfo> info = std::make_shared<SectorInfo>();
    SectorNumber sector = 1;
    info->sector_number = sector;
    info->seed_epoch = kPreCommitChallengeDelay;
    info->message = "010001020001"_cid;
    info->comm_r = "010001020005"_cid;
    info->comm_d = "010001020006"_cid;
    info->seed = Randomness{{1, 2, 3, 4, 5}};

    Proof proof{{1, 2, 3}};

    TipsetKey commit_key{{CbCid::hash("01"_unhex),
                          CbCid::hash("02"_unhex),
                          CbCid::hash("03"_unhex)}};
    auto actor_key{"010001020003"_cid};
    SectorPreCommitOnChainInfo some_info;
    some_info.info.sealed_cid = info->comm_r.value();
    some_info.info.seal_epoch = info->ticket_epoch + 1;
    some_info.precommit_epoch = info->seed_epoch - kPreCommitChallengeDelay;
    EXPECT_OUTCOME_TRUE_1(
        actor_state_->precommitted_sectors.set(sector, some_info));
    EXPECT_OUTCOME_TRUE(cid_root,
                        actor_state_->precommitted_sectors.hamt.flush());

    api_->ChainReadObj = [&](CID key) -> outcome::result<Buffer> {
      if (key == actor_key) {
        return codec::cbor::encode(actor_state_);
      }
      if (key == cid_root) {
        EXPECT_OUTCOME_TRUE(root, getCbor<storage::hamt::Node>(ipld_, cid_root));
        return codec::cbor::encode(root);
      }
      if (key == actor_state_->allocated_sectors) {
        return codec::cbor::encode(primitives::RleBitset());
      }
      return ERROR_TEXT("ERROR");
    };

    Actor actor;
    actor.code = vm::actor::builtin::v0::kStorageMinerCodeId;
    actor.head = actor_key;
    api_->StateGetActor = [&](const Address &addr, const TipsetKey &tipset_key)
        -> outcome::result<Actor> { return actor; };

    api_->ChainGetRandomnessFromBeacon =
        [&](const TipsetKey &key,
            DomainSeparationTag tag,
            ChainEpoch epoch,
            const Buffer &buf) -> outcome::result<Randomness> {
      if (key == commit_key
          and tag == DomainSeparationTag::InteractiveSealChallengeSeed
          and epoch == info->seed_epoch) {
        return info->seed;
      }

      return ERROR_TEXT("ERROR");
    };

    EXPECT_CALL(*proofs_, verifySeal(_))
        .WillOnce(testing::Return(outcome::success(false)));
    EXPECT_OUTCOME_ERROR(
        ChecksError::kInvalidProof,
        checkCommit(miner_addr_, info, proof, commit_key, api_, proofs_));
  }

}  // namespace fc::mining::checks
