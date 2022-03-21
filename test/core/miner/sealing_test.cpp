/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "core/miner/sealing_test_fixture.hpp"

namespace fc::mining {

  class SealingTest : public SealingTestFixture {};

  /**
   * @given address
   * @when try to get address
   * @then address was getted
   */
  TEST_F(SealingTest, getAddress) {
    ASSERT_EQ(miner_addr_, sealing_->getAddress());
  }

  /**
   * @given nothing
   * @when try to get not exist sector
   * @then SealingError::kCannotFindSector occurs
   */
  TEST_F(SealingTest, getSectorInfoNotFound) {
    EXPECT_OUTCOME_ERROR(SealingError::kCannotFindSector,
                         sealing_->getSectorInfo(1));
  }

  /**
   * @given nothing
   * @when try to remove not exist sector
   * @then SealingError::kCannotFindSector occurs
   */
  TEST_F(SealingTest, RemoveNotFound) {
    EXPECT_OUTCOME_ERROR(SealingError::kCannotFindSector, sealing_->remove(1));
  }

  /**
   * @given sector(in Proving state)
   * @when try to remove
   * @then sector was removed
   */
  TEST_F(SealingTest, Remove) {
    UnpaddedPieceSize piece_size(127);
    PieceData piece("/dev/random");
    DealInfo deal{
        .publish_cid = "010001020001"_cid,
        .deal_id = 0,
        .deal_schedule =
            {
                .start_epoch = 0,
                .end_epoch = 1,
            },
        .is_keep_unsealed = true,
    };

    SectorNumber sector = 1;
    EXPECT_CALL(*counter_, next()).WillOnce(testing::Return(sector));

    PieceInfo info{
        .size = piece_size.padded(),
        .cid = "010001020001"_cid,
    };

    SectorRef sector_ref{
        .id = SectorId{.miner = miner_id_, .sector = sector},
        .proof_type = seal_proof_type_,
    };
    EXPECT_CALL(*manager_,
                doAddPieceSync(
                    sector_ref, IsEmpty(), piece_size, _, kDealSectorPriority))
        .WillOnce(testing::Return(info));

    EXPECT_OUTCOME_TRUE_1(
        sealing_->addPieceToAnySector(piece_size, std::move(piece), deal));

    EXPECT_OUTCOME_TRUE(info_before, sealing_->getSectorInfo(sector));
    EXPECT_EQ(info_before->state, SealingState::kStateUnknown);
    EXPECT_OUTCOME_TRUE_1(
        sealing_->forceSectorState(sector, SealingState::kProving));
    EXPECT_OUTCOME_TRUE_1(sealing_->remove(sector));

    EXPECT_CALL(*manager_, remove(sector_ref))
        .WillOnce(testing::Return(outcome::success()));

    runForSteps(*context_, 100);

    EXPECT_OUTCOME_TRUE(sector_info, sealing_->getSectorInfo(sector));
    EXPECT_EQ(sector_info->state, SealingState::kRemoved);
  }

  /**
   * @given piece, deal(not published)
   * @when try to add piece to Sector
   * @then SealingError::kNotPublishedDeal occurs
   */
  TEST_F(SealingTest, addPieceToAnySectorNotPublishedDeal) {
    UnpaddedPieceSize piece_size(127);
    PieceData piece("/dev/random");
    DealInfo deal{
        .publish_cid = boost::none,
        .deal_id = 0,
        .deal_schedule =
            {
                .start_epoch = 0,
                .end_epoch = 1,
            },
        .is_keep_unsealed = true,
    };
    EXPECT_OUTCOME_ERROR(
        SealingError::kNotPublishedDeal,
        sealing_->addPieceToAnySector(piece_size, std::move(piece), deal));
  }

  /**
   * @given piece(size not valid)
   * @when try to add piece to Sector
   * @then SealingError::kCannotAllocatePiece occurs
   */
  TEST_F(SealingTest, addPieceToAnySectorCannotAllocatePiece) {
    UnpaddedPieceSize piece_size(128);
    PieceData piece("/dev/random");
    DealInfo deal{
        .publish_cid = "010001020001"_cid,
        .deal_id = 0,
        .deal_schedule =
            {
                .start_epoch = 0,
                .end_epoch = 1,
            },
        .is_keep_unsealed = true,
    };
    EXPECT_OUTCOME_ERROR(
        SealingError::kCannotAllocatePiece,
        sealing_->addPieceToAnySector(piece_size, std::move(piece), deal));
  }

  /**
   * @given large piece(size > sector size)
   * @when try to add piece to Sector
   * @then SealingError::kPieceNotFit occurs
   */
  TEST_F(SealingTest, addPieceToAnySectorPieceNotFit) {
    UnpaddedPieceSize piece_size(4064);
    PieceData piece("/dev/random");
    DealInfo deal{
        .publish_cid = "010001020001"_cid,
        .deal_id = 0,
        .deal_schedule =
            {
                .start_epoch = 0,
                .end_epoch = 1,
            },
        .is_keep_unsealed = true,
    };

    EXPECT_OUTCOME_ERROR(
        SealingError::kPieceNotFit,
        sealing_->addPieceToAnySector(piece_size, std::move(piece), deal));
  }

  /**
   * @given piece
   * @when try to add piece to Sector
   * @then success and state is WaitDeals
   */
  TEST_F(SealingTest, addPieceToAnySectorWithoutStartPacking) {
    UnpaddedPieceSize piece_size(127);
    PieceData piece("/dev/random");
    DealInfo deal{
        .publish_cid = "010001020001"_cid,
        .deal_id = 0,
        .deal_schedule =
            {
                .start_epoch = 0,
                .end_epoch = 1,
            },
        .is_keep_unsealed = true,
    };

    SectorNumber sector = 1;
    EXPECT_CALL(*counter_, next()).WillOnce(testing::Return(sector));

    PieceInfo info{
        .size = piece_size.padded(),
        .cid = "010001020001"_cid,
    };

    SectorRef sector_ref{.id = SectorId{.miner = miner_id_, .sector = sector},
                         .proof_type = seal_proof_type_};
    EXPECT_CALL(*manager_,
                doAddPieceSync(
                    sector_ref, IsEmpty(), piece_size, _, kDealSectorPriority))
        .WillOnce(testing::Return(info));

    EXPECT_OUTCOME_TRUE(
        piece_attribute,
        sealing_->addPieceToAnySector(piece_size, std::move(piece), deal));
    EXPECT_EQ(piece_attribute.sector, sector);
    EXPECT_EQ(piece_attribute.offset, 0);
    EXPECT_EQ(piece_attribute.size.unpadded(), piece_size);

    runForSteps(*context_, 100);

    EXPECT_OUTCOME_TRUE(sector_info, sealing_->getSectorInfo(sector));
    EXPECT_EQ(sector_info->sector_number, sector);
    EXPECT_EQ(sector_info->state, SealingState::kWaitDeals);
  }

  /**
   * @given sector in sealing
   * @when try to get List of sectors
   * @then list size is 2
   */
  TEST_F(SealingTest, ListOfSectors) {
    UnpaddedPieceSize piece_size(127);
    PieceData piece("/dev/random");
    DealInfo deal{
        .publish_cid = "010001020001"_cid,
        .deal_id = 0,
        .deal_schedule =
            {
                .start_epoch = 0,
                .end_epoch = 1,
            },
        .is_keep_unsealed = true,
    };

    SectorNumber sector = 1;
    EXPECT_CALL(*counter_, next()).WillOnce(testing::Return(sector));

    PieceInfo info{
        .size = piece_size.padded(),
        .cid = "010001020001"_cid,
    };

    SectorRef sector_ref{.id = SectorId{.miner = miner_id_, .sector = sector},
                         .proof_type = seal_proof_type_};
    EXPECT_CALL(*manager_,
                doAddPieceSync(
                    sector_ref, IsEmpty(), piece_size, _, kDealSectorPriority))
        .WillOnce(testing::Return(info));

    EXPECT_OUTCOME_TRUE_1(
        sealing_->addPieceToAnySector(piece_size, std::move(piece), deal));

    auto sectors = sealing_->getListSectors();
    ASSERT_EQ(sectors.size(), 2);
  }

  /**
   * @given sector
   * @when try to seal sector to Proving
   * @then success
   */
  TEST_F(SealingTest, processToProving) {
    UnpaddedPieceSize piece_size(2032);
    PieceData piece("/dev/random");
    DealInfo deal{
        .publish_cid = "010001020001"_cid,
        .deal_id = 0,
        .deal_schedule =
            {
                .start_epoch = 1,
                .end_epoch = 2,
            },
        .is_keep_unsealed = true,
    };

    SectorNumber sector = 1;
    EXPECT_CALL(*counter_, next()).WillOnce(testing::Return(sector));

    SectorId sector_id{.miner = miner_id_, .sector = sector};

    PieceInfo info{
        .size = piece_size.padded(),
        .cid = "010001020001"_cid,
    };

    SectorRef sector_ref{.id = sector_id, .proof_type = seal_proof_type_};
    EXPECT_CALL(*manager_,
                doAddPieceSync(
                    sector_ref, IsEmpty(), piece_size, _, kDealSectorPriority))
        .WillOnce(testing::Return(info));

    EXPECT_OUTCOME_TRUE_1(
        sealing_->addPieceToAnySector(piece_size, std::move(piece), deal));

    // Precommit 1
    TipsetKey key{{CbCid::hash("02"_unhex)}};
    auto tipset = std::make_shared<Tipset>(key, std::vector<BlockHeader>());

    api_->ChainHead = [&]() -> outcome::result<TipsetCPtr> { return tipset; };

    DealProposal proposal;
    proposal.piece_cid = info.cid;
    proposal.piece_size = info.size;
    proposal.start_epoch = tipset->height() + 1;
    proposal.provider = miner_addr_;
    StorageDeal storage_deal;
    storage_deal.proposal = proposal;

    api_->StateMarketStorageDeal =
        [&](DealId deal_id,
            const TipsetKey &tipset_key) -> outcome::result<StorageDeal> {
      if (deal_id == deal.deal_id and tipset_key == key) {
        return storage_deal;
      }
      return ERROR_TEXT("ERROR");
    };

    auto actor_key{"010001020003"_cid};
    auto ipld{std::make_shared<InMemoryDatastore>()};
    const auto actor_version = actorVersion(version_);
    ipld->actor_version = actor_version;
    auto actor_state = makeMinerActorState(ipld, actor_version);
    SectorPreCommitOnChainInfo some_info;
    some_info.info.sealed_cid = "010001020006"_cid;
    EXPECT_OUTCOME_TRUE_1(
        actor_state->precommitted_sectors.set(sector + 1, some_info));
    EXPECT_OUTCOME_TRUE(cid_root,
                        actor_state->precommitted_sectors.hamt.flush());
    api_->ChainReadObj = [&](CID key) -> outcome::result<Bytes> {
      if (key == actor_key) {
        return codec::cbor::encode(actor_state);
      }
      if (key == cid_root) {
        EXPECT_OUTCOME_TRUE(root, getCbor<storage::hamt::Node>(ipld, cid_root));
        return codec::cbor::encode(root);
      }
      if (key == actor_state->allocated_sectors) {
        return codec::cbor::encode(primitives::RleBitset());
      }
      return ERROR_TEXT("ERROR");
    };

    Actor actor;
    actor.code = vm::actor::builtin::v0::kStorageMinerCodeId;
    actor.head = actor_key;
    api_->StateGetActor = [&](const Address &addr, const TipsetKey &tipset_key)
        -> outcome::result<Actor> { return actor; };

    Randomness rand{{1, 2, 3}};

    api_->ChainGetRandomnessFromTickets =
        [&](const TipsetKey &,
            DomainSeparationTag,
            ChainEpoch,
            const Bytes &) -> outcome::result<Randomness> { return rand; };

    std::vector<PieceInfo> infos = {info};

    types::PreCommit1Output pc1o({4, 5, 6});

    EXPECT_CALL(*manager_,
                sealPreCommit1(sector_ref, rand, infos, _, kDealSectorPriority))
        .WillOnce(testing::Invoke(
            [=](auto, auto, auto, auto cb, auto) { cb(pc1o); }));

    // Precommit 2

    sector_storage::SectorCids cids{.sealed_cid = "010001020010"_cid,
                                    .unsealed_cid = "010001020011"_cid};

    EXPECT_CALL(*manager_,
                sealPreCommit2(sector_ref, pc1o, _, kDealSectorPriority))
        .WillOnce(
            testing::Invoke([=](auto, auto, auto cb, auto) { cb(cids); }));

    // Precommitting

    api_->StateCall =
        [&](const UnsignedMessage &message,
            const TipsetKey &tipset_key) -> outcome::result<InvocResult> {
      InvocResult result;
      ComputeDataCommitment::Result call_res{.commds = {cids.unsealed_cid}};
      EXPECT_OUTCOME_TRUE(unsealed_buffer, codec::cbor::encode(call_res));
      result.receipt = MessageReceipt{
          .exit_code = vm::VMExitCode::kOk,
          .return_value = unsealed_buffer,
      };
      return result;
    };

    EXPECT_CALL(*policy_, expiration(_))
        .WillOnce(testing::Return(outcome::success(0)));

    api_->StateMinerPreCommitDepositForPower =
        [](const Address &,
           const SectorPreCommitInfo &,
           const TipsetKey &) -> outcome::result<TokenAmount> { return 10; };

    CID precommit_msg_cid = "010001020042"_cid;
    EXPECT_CALL(*precommit_batcher_, addPreCommit(_, _, _, _))
        .WillOnce(testing::Invoke(
            [=](const SectorInfo &,
                const TokenAmount &,
                const SectorPreCommitInfo &,
                const PrecommitCallback &cb) -> outcome::result<void> {
              cb(precommit_msg_cid);
              return outcome::success();
            }));

    CID commit_msg_cid;  // for commit stage
    api_->MpoolPushMessage = [&commit_msg_cid](
                                 const UnsignedMessage &msg,
                                 const boost::optional<api::MessageSendSpec> &)
        -> outcome::result<SignedMessage> {
      commit_msg_cid = msg.getCid();
      return SignedMessage{.message = msg, .signature = BlsSignature()};
    };

    // Precommitted

    TipsetKey precommit_tipset_key{
        {CbCid::hash("11"_unhex), CbCid::hash("12"_unhex)}};
    TipsetKey commit_tipset_key{
        {CbCid::hash("13"_unhex), CbCid::hash("14"_unhex)}};
    EpochDuration height = 3;
    api_->StateWaitMsg =
        [&](const CID &msg_cid, auto, auto, auto) -> outcome::result<MsgWait> {
      if (msg_cid == precommit_msg_cid) {
        // make precommit for actor
        {
          SectorPreCommitOnChainInfo new_info;
          new_info.precommit_epoch = height;
          new_info.info.sealed_cid = cids.sealed_cid;
          EXPECT_OUTCOME_TRUE_1(
              actor_state->precommitted_sectors.set(sector, new_info));
          EXPECT_OUTCOME_TRUE(cid_root_res,
                              actor_state->precommitted_sectors.hamt.flush());
          cid_root = std::move(cid_root_res);
        }

        MsgWait result;
        result.tipset = precommit_tipset_key;
        result.receipt.exit_code = vm::VMExitCode::kOk;
        return result;
      }
      if (msg_cid == commit_msg_cid) {
        MsgWait result;
        result.tipset = commit_tipset_key;
        result.receipt.exit_code = vm::VMExitCode::kOk;
        return result;
      }

      return ERROR_TEXT("ERROR");
    };

    // Wait Seed

    Randomness seed{{6, 7, 8, 9}};
    api_->ChainGetRandomnessFromBeacon =
        [&](const TipsetKey &,
            DomainSeparationTag,
            ChainEpoch,
            const Bytes &) -> outcome::result<Randomness> { return seed; };

    EXPECT_CALL(*events_,
                chainAt(_,
                        _,
                        kInteractivePoRepConfidence,
                        height + kPreCommitChallengeDelay))
        .WillOnce(testing::Invoke(
            [](auto apply, auto, auto, auto) -> outcome::result<void> {
              EXPECT_OUTCOME_TRUE_1(apply({}, 0));
              return outcome::success();
            }));

    // Compute Proofs

    Commit1Output c1o({1, 2, 3, 4, 5, 6});
    EXPECT_CALL(
        *manager_,
        sealCommit1(
            sector_ref, rand, seed, infos, cids, _, kDealSectorPriority))
        .WillOnce(testing::Invoke(
            [=](auto, auto, auto, auto, auto, auto cb, auto) { cb(c1o); }));
    Proof proof({7, 6, 5, 4, 3, 2, 1});
    EXPECT_CALL(*manager_, sealCommit2(sector_ref, c1o, _, kDealSectorPriority))
        .WillOnce(
            testing::Invoke([=](auto, auto, auto cb, auto) { cb(proof); }));

    // Commiting
    EXPECT_CALL(*proofs_, verifySeal(_))
        .WillOnce(testing::Return(outcome::success(true)))
        .WillOnce(testing::Return(outcome::success(true)));

    api_->StateMinerInitialPledgeCollateral =
        [](const Address &,
           const SectorPreCommitInfo &,
           const TipsetKey &) -> outcome::result<TokenAmount> { return 0; };

    // Commit Wait

    api_->StateSectorGetInfo =
        [&](const Address &, SectorNumber, const TipsetKey &key)
        -> outcome::result<boost::optional<SectorOnChainInfo>> {
      if (key == commit_tipset_key) {
        return SectorOnChainInfo{};
      }

      return ERROR_TEXT("ERROR");
    };

    // Finalize
    EXPECT_CALL(*manager_,
                finalizeSector(sector_ref, _, _, kDealSectorPriority))
        .WillOnce(testing::Invoke(
            [=](auto, auto, auto cb, auto) { cb(outcome::success()); }));

    auto state{SealingState::kStateUnknown};
    while (state != SealingState::kProving) {
      runForSteps(*context_, 100);
      EXPECT_OUTCOME_TRUE(sector_info, sealing_->getSectorInfo(sector));
      ASSERT_NE(sector_info->state, state);
      state = sector_info->state;
    }
  }

  /**
   * @given sealing, 1 sector
   * @when try to add pledge sector
   * @then 2 sectors in sealing
   */
  TEST_F(SealingTest, pledgeSector) {
    SectorNumber sector = 1;
    EXPECT_CALL(*counter_, next()).WillOnce(testing::Return(sector));

    SectorId sector_id{
        .miner = 42,
        .sector = sector,
    };

    PieceInfo info{
        .size = PaddedPieceSize(sector_size_),
        .cid = "010001020002"_cid,
    };

    SectorRef sector_ref{.id = sector_id, .proof_type = seal_proof_type_};
    std::vector<UnpaddedPieceSize> exist_pieces = {};
    EXPECT_CALL(*manager_,
                doAddNullPiece(sector_ref,
                               exist_pieces,
                               PaddedPieceSize(sector_size_).unpadded(),
                               _,
                               0))
        .WillOnce(testing::Invoke(
            [&info](auto, auto, auto, auto cb, auto) { cb(info); }));

    ASSERT_EQ(sealing_->getListSectors().size(), 1);
    EXPECT_OUTCOME_TRUE_1(sealing_->pledgeSector());
    scheduler_backend_->shiftToTimer();
    ASSERT_EQ(sealing_->getListSectors().size(), 2);
  }

}  // namespace fc::mining
