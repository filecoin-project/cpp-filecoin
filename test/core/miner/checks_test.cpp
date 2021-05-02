/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "miner/storage_fsm/impl/checks.hpp"

#include <gtest/gtest.h>

#include "storage/ipfs/impl/in_memory_datastore.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"
#include "vm/actor/actor.hpp"
#include "vm/actor/builtin/types/miner/sector_info.hpp"
#include "vm/actor/builtin/v0/codes.hpp"
#include "vm/actor/builtin/v0/market/market_actor.hpp"
#include "vm/actor/builtin/v0/miner/miner_actor_state.hpp"
#include "vm/exit_code/exit_code.hpp"

namespace fc::mining::checks {
  using api::InvocResult;
  using api::SectorNumber;
  using api::SectorOnChainInfo;
  using api::UnsignedMessage;
  using primitives::tipset::Tipset;
  using primitives::tipset::TipsetCPtr;
  using storage::ipfs::InMemoryDatastore;
  using types::DealInfo;
  using types::DealSchedule;
  using types::PaddedPieceSize;
  using types::Piece;
  using types::PieceInfo;
  using vm::VMExitCode;
  using vm::actor::Actor;
  using vm::actor::builtin::v0::market::ComputeDataCommitment;
  using vm::actor::builtin::v0::miner::MinerActorState;

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

  class CheckPrecommit : public testing::Test {
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

    std::vector<CID> cids = {
        "010001020001"_cid, "010001020002"_cid, "010001020003"_cid};
    TipsetKey precommit_key(cids);
    api_->StateCall =
        [&precommit_key](const UnsignedMessage &msg,
                         const TipsetKey &key) -> outcome::result<InvocResult> {
      if (msg.method == ComputeDataCommitment::Number
          and key == precommit_key) {
        InvocResult res;
        res.receipt.exit_code = VMExitCode::kOk;
        OUTCOME_TRY(cid_buf, codec::cbor::encode("010001020002"_cid));
        res.receipt.return_value = cid_buf;
        return res;
      }

      return ERROR_TEXT("ERROR");
    };

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

    std::vector<CID> cids = {
        "010001020001"_cid, "010001020002"_cid, "010001020003"_cid};
    TipsetKey precommit_key(cids);
    api_->StateCall =
        [&precommit_key, &info](
            const UnsignedMessage &msg,
            const TipsetKey &key) -> outcome::result<InvocResult> {
      if (msg.method == ComputeDataCommitment::Number
          and key == precommit_key) {
        InvocResult res;
        res.receipt.exit_code = VMExitCode::kOk;
        OUTCOME_TRY(cid_buf, codec::cbor::encode(*info->comm_d));
        res.receipt.return_value = cid_buf;
        return res;
      }

      return ERROR_TEXT("ERROR");
    };

    api_->StateNetworkVersion =
        [&precommit_key](
            const TipsetKey &key) -> outcome::result<NetworkVersion> {
      if (key == precommit_key) {
        return NetworkVersion::kVersion0;
      }

      return ERROR_TEXT("ERROR");
    };

    EXPECT_OUTCOME_TRUE(
        duration,
        vm::actor::builtin::types::miner::maxSealDuration(info->sector_type));
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

    std::vector<CID> cids = {
        "010001020001"_cid, "010001020002"_cid, "010001020003"_cid};
    TipsetKey precommit_key(cids);
    api_->StateCall =
        [&precommit_key, &info](
            const UnsignedMessage &msg,
            const TipsetKey &key) -> outcome::result<InvocResult> {
      if (msg.method == ComputeDataCommitment::Number
          and key == precommit_key) {
        InvocResult res;
        res.receipt.exit_code = VMExitCode::kOk;
        OUTCOME_TRY(cid_buf, codec::cbor::encode(*info->comm_d));
        res.receipt.return_value = cid_buf;
        return res;
      }

      return ERROR_TEXT("ERROR");
    };

    api_->StateNetworkVersion =
        [&precommit_key](
            const TipsetKey &key) -> outcome::result<NetworkVersion> {
      if (key == precommit_key) {
        return NetworkVersion::kVersion0;
      }

      return ERROR_TEXT("ERROR");
    };

    EXPECT_OUTCOME_TRUE(
        duration,
        vm::actor::builtin::types::miner::maxSealDuration(info->sector_type));
    ChainEpoch height = duration
                        + vm::actor::builtin::types::miner::kChainFinality
                        + info->ticket_epoch;

    auto actor_key{"010001020003"_cid};
    auto ipld{std::make_shared<InMemoryDatastore>()};
    MinerActorState actor_state;
    actor_state.miner_info = "010001020004"_cid;
    actor_state.vesting_funds = "010001020004"_cid;
    actor_state.allocated_sectors = "010001020004"_cid;
    actor_state.deadlines = "010001020006"_cid;
    actor_state.precommitted_sectors =
        adt::Map<SectorPreCommitOnChainInfo, adt::UvarintKeyer>(ipld);
    SectorPreCommitOnChainInfo some_info;
    some_info.info.sealed_cid = "010001020006"_cid;
    EXPECT_OUTCOME_TRUE_1(
        actor_state.precommitted_sectors.set(sector, some_info));
    EXPECT_OUTCOME_TRUE(cid_root,
                        actor_state.precommitted_sectors.hamt.flush());
    SectorPreCommitOnChainInfo precommit_info;
    precommit_info.info.seal_epoch = 3;
    precommit_info.info.sealed_cid = "010001020007"_cid;
    actor_state.sectors =
        adt::Array<SectorOnChainInfo>("010001020008"_cid, ipld);
    actor_state.precommitted_setctors_expiry =
        adt::Array<api::RleBitset>("010001020009"_cid, ipld);
    api_->ChainReadObj = [&](CID key) -> outcome::result<Buffer> {
      if (key == actor_key) {
        return codec::cbor::encode(actor_state);
      }
      if (key == cid_root) {
        OUTCOME_TRY(root, ipld->getCbor<storage::hamt::Node>(cid_root));
        return codec::cbor::encode(root);
      }
      if (key == actor_state.allocated_sectors) {
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

    std::vector<CID> cids = {
        "010001020001"_cid, "010001020002"_cid, "010001020003"_cid};
    TipsetKey precommit_key(cids);
    api_->StateCall =
        [&precommit_key, &info](
            const UnsignedMessage &msg,
            const TipsetKey &key) -> outcome::result<InvocResult> {
      if (msg.method == ComputeDataCommitment::Number
          and key == precommit_key) {
        InvocResult res;
        res.receipt.exit_code = VMExitCode::kOk;
        OUTCOME_TRY(cid_buf, codec::cbor::encode(*info->comm_d));
        res.receipt.return_value = cid_buf;
        return res;
      }

      return ERROR_TEXT("ERROR");
    };

    api_->StateNetworkVersion =
        [&precommit_key](
            const TipsetKey &key) -> outcome::result<NetworkVersion> {
      if (key == precommit_key) {
        return NetworkVersion::kVersion0;
      }

      return ERROR_TEXT("ERROR");
    };

    EXPECT_OUTCOME_TRUE(
        duration,
        vm::actor::builtin::types::miner::maxSealDuration(info->sector_type));
    ChainEpoch height = duration
                        + vm::actor::builtin::types::miner::kChainFinality
                        + info->ticket_epoch;

    auto actor_key{"010001020003"_cid};
    auto ipld{std::make_shared<InMemoryDatastore>()};
    MinerActorState actor_state;
    actor_state.miner_info = "010001020004"_cid;
    actor_state.vesting_funds = "010001020004"_cid;
    actor_state.allocated_sectors = "010001020004"_cid;
    actor_state.deadlines = "010001020006"_cid;
    actor_state.precommitted_sectors =
        adt::Map<SectorPreCommitOnChainInfo, adt::UvarintKeyer>(ipld);
    SectorPreCommitOnChainInfo some_info;
    some_info.info.sealed_cid = "010001020006"_cid;
    some_info.info.seal_epoch = info->ticket_epoch + 1;
    EXPECT_OUTCOME_TRUE_1(
        actor_state.precommitted_sectors.set(sector, some_info));
    EXPECT_OUTCOME_TRUE(cid_root,
                        actor_state.precommitted_sectors.hamt.flush());
    SectorPreCommitOnChainInfo precommit_info;
    precommit_info.info.seal_epoch = 3;
    precommit_info.info.sealed_cid = "010001020007"_cid;
    actor_state.sectors =
        adt::Array<SectorOnChainInfo>("010001020008"_cid, ipld);
    actor_state.precommitted_setctors_expiry =
        adt::Array<api::RleBitset>("010001020009"_cid, ipld);
    api_->ChainReadObj = [&](CID key) -> outcome::result<Buffer> {
      if (key == actor_key) {
        return codec::cbor::encode(actor_state);
      }
      if (key == cid_root) {
        OUTCOME_TRY(root, ipld->getCbor<storage::hamt::Node>(cid_root));
        return codec::cbor::encode(root);
      }
      if (key == actor_state.allocated_sectors) {
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

}  // namespace fc::mining::checks
