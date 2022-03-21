/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "miner/storage_fsm/impl/sealing_impl.hpp"

#include <gtest/gtest.h>
#include <chrono>
#include <libp2p/basic/scheduler/manual_scheduler_backend.hpp>
#include <libp2p/basic/scheduler/scheduler_impl.hpp>

#include "miner/address_selector.hpp"
#include "primitives/sector/sector.hpp"
#include "storage/in_memory/in_memory_storage.hpp"
#include "storage/ipfs/impl/in_memory_datastore.hpp"
#include "testutil/context_wait.hpp"
#include "testutil/default_print.hpp"
#include "testutil/literals.hpp"
#include "testutil/mocks/api.hpp"
#include "testutil/mocks/miner/events_mock.hpp"
#include "testutil/mocks/miner/precommit_batcher_mock.hpp"
#include "testutil/mocks/miner/precommit_policy_mock.hpp"
#include "testutil/mocks/primitives/stored_counter_mock.hpp"
#include "testutil/mocks/proofs/proof_engine_mock.hpp"
#include "testutil/mocks/sector_storage/manager_mock.hpp"
#include "testutil/outcome.hpp"
#include "testutil/vm/actor/builtin/actor_test_util.hpp"
#include "vm/actor/builtin/v5/market/market_actor.hpp"
#include "vm/actor/codes.hpp"

namespace fc::mining {

  using adt::Channel;
  using api::DealId;
  using api::DomainSeparationTag;
  using api::InvocResult;
  using api::MsgWait;
  using api::StorageDeal;
  using api::TipsetCPtr;
  using api::UnsignedMessage;
  using crypto::randomness::Randomness;
  using fc::storage::ipfs::InMemoryDatastore;
  using libp2p::basic::ManualSchedulerBackend;
  using libp2p::basic::Scheduler;
  using libp2p::basic::SchedulerImpl;
  using markets::storage::DealProposal;
  using mining::SelectAddress;
  using primitives::CounterMock;
  using primitives::block::BlockHeader;
  using primitives::sector::Proof;
  using primitives::tipset::Tipset;
  using sector_storage::Commit1Output;
  using sector_storage::ManagerMock;
  using storage::InMemoryStorage;
  using testing::_;
  using testing::IsEmpty;
  using types::kDealSectorPriority;
  using types::Piece;
  using types::SectorInfo;
  using vm::actor::Actor;
  using vm::actor::builtin::makeMinerActorState;
  using vm::actor::builtin::types::miner::kPreCommitChallengeDelay;
  using vm::actor::builtin::types::miner::SectorOnChainInfo;
  using vm::actor::builtin::v5::market::ComputeDataCommitment;
  using vm::message::SignedMessage;
  using vm::runtime::MessageReceipt;

  /**
   * Fixture for sealing tests, see .cpp files.
   */
  class SealingTestFixture : public testing::Test {
   protected:
    void SetUp() override {
      auto sector_size = primitives::sector::getSectorSize(seal_proof_type_);
      ASSERT_FALSE(sector_size.has_error());
      sector_size_ = PaddedPieceSize(sector_size.value());

      events_ = std::make_shared<EventsMock>();
      miner_addr_ = Address::makeFromId(miner_id_);
      counter_ = std::make_shared<CounterMock>();
      kv_ = std::make_shared<InMemoryStorage>();

      SectorInfo info;
      info.sector_number = update_sector_id_;
      info.state = SealingState::kProving;
      info.pieces = {Piece{
          .piece =
              PieceInfo{
                  .size = PaddedPieceSize(sector_size_),
                  .cid = "010001020011"_cid,
              },
          .deal_info = boost::none,
      }};
      EXPECT_OUTCOME_TRUE(buf, codec::cbor::encode(info));
      const std::string string_key = "empty_sector";
      const Bytes key(string_key.begin(), string_key.end());
      EXPECT_OUTCOME_TRUE_1(kv_->put(key, std::move(buf)));

      proofs_ = std::make_shared<proofs::ProofEngineMock>();

      manager_ = std::make_shared<ManagerMock>();

      EXPECT_CALL(*manager_, getProofEngine())
          .WillRepeatedly(testing::Return(proofs_));

      policy_ = std::make_shared<PreCommitPolicyMock>();
      context_ = std::make_shared<boost::asio::io_context>();

      config_ = Config{
          .max_wait_deals_sectors = 2,
          .max_sealing_sectors = 0,
          .max_sealing_sectors_for_deals = 0,
          .wait_deals_delay = std::chrono::hours(6),
          .batch_pre_commits = true,
      };

      fee_config_ = std::make_shared<FeeConfig>();
      fee_config_->max_precommit_batch_gas_fee.per_sector =
          TokenAmount{"2000000000000000"};
      fee_config_->max_precommit_gas_fee = TokenAmount{"25000000000000000"};

      scheduler_backend_ = std::make_shared<ManualSchedulerBackend>();
      scheduler_ = std::make_shared<SchedulerImpl>(scheduler_backend_,
                                                   Scheduler::Config{});
      precommit_batcher_ = std::make_shared<PreCommitBatcherMock>();

      EXPECT_OUTCOME_TRUE(sealing,
                          SealingImpl::newSealing(
                              api_,
                              events_,
                              miner_addr_,
                              counter_,
                              kv_,
                              manager_,
                              policy_,
                              context_,
                              scheduler_,
                              precommit_batcher_,
                              [=](const MinerInfo &miner_info,
                                  const TokenAmount &good_funds,
                                  const std::shared_ptr<FullNodeApi> &api)
                                  -> outcome::result<Address> {
                                return SelectAddress(
                                    miner_info, good_funds, api);
                              },
                              fee_config_,
                              config_));
      sealing_ = sealing;

      MinerInfo minfo;
      minfo.window_post_proof_type =
          primitives::sector::RegisteredPoStProof::kStackedDRG2KiBWindowPoSt;
      EXPECT_CALL(mock_StateMinerInfo, Call(miner_addr_, _))
          .WillRepeatedly(testing::Return(minfo));
      version_ = NetworkVersion::kVersion13;
      EXPECT_CALL(mock_StateNetworkVersion, Call(_))
          .WillRepeatedly(testing::Return(version_));
    }

    void TearDown() override {
      context_->stop();
    }

    uint64_t update_sector_id_{2};
    RegisteredSealProof seal_proof_type_{
        RegisteredSealProof::kStackedDrg2KiBV1_1};

    PaddedPieceSize sector_size_;
    Config config_;
    std::shared_ptr<FeeConfig> fee_config_;
    std::shared_ptr<FullNodeApi> api_ = std::make_shared<FullNodeApi>();
    std::shared_ptr<EventsMock> events_;
    uint64_t miner_id_{42};
    Address miner_addr_;
    std::shared_ptr<CounterMock> counter_;
    std::shared_ptr<InMemoryStorage> kv_;
    std::shared_ptr<ManagerMock> manager_;
    std::shared_ptr<proofs::ProofEngineMock> proofs_;
    std::shared_ptr<PreCommitPolicyMock> policy_;
    std::shared_ptr<boost::asio::io_context> context_;
    std::shared_ptr<ManualSchedulerBackend> scheduler_backend_;
    std::shared_ptr<Scheduler> scheduler_;
    NetworkVersion version_;

    std::shared_ptr<Sealing> sealing_;
    std::shared_ptr<PreCommitBatcherMock> precommit_batcher_;
    MOCK_API(api_, StateMinerInfo);
    MOCK_API(api_, StateNetworkVersion);
  };
}  // namespace fc::mining
