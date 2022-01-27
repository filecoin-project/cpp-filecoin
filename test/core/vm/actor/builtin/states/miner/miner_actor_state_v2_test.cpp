/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/states/miner/v2/miner_actor_state.hpp"

#include <gtest/gtest.h>
#include "core/vm/actor/builtin/types/miner/expected_deadline_v2.hpp"
#include "core/vm/actor/builtin/types/miner/test_utils.hpp"
#include "storage/ipfs/impl/in_memory_datastore.hpp"
#include "testutil/literals.hpp"
#include "testutil/mocks/vm/runtime/runtime_mock.hpp"
#include "testutil/outcome.hpp"
#include "vm/actor/builtin/types/miner/bitfield_queue.hpp"
#include "vm/actor/builtin/types/miner/policy.hpp"
#include "vm/version/version.hpp"

namespace fc::vm::actor::builtin::v2::miner {
  using runtime::MockRuntime;
  using states::kPrecommitExpiryBitwidth;
  using storage::ipfs::InMemoryDatastore;
  using namespace types::miner;

  struct VestingTestCase {
    VestSpec vspec;
    ChainEpoch period_start{};
    std::vector<int64_t> vepochs;
  };

  struct MinerActorStateTestV2 : testing::Test {
    void SetUp() override {
      actor_version = ActorVersion::kVersion2;
      ipld->actor_version = actor_version;
      cbor_blake::cbLoadT(ipld, state);

      EXPECT_CALL(runtime, getIpfsDatastore())
          .WillRepeatedly(testing::Invoke([&]() { return ipld; }));

      EXPECT_CALL(runtime, getActorVersion())
          .WillRepeatedly(testing::Invoke([&]() { return actor_version; }));

      epoch = 10;
      sector_num = 1;
      vest_start = 10;
      vest_sum = 100;

      initState();
    }

    void initState() {
      std::vector<Multiaddress> multiaddresses;
      multiaddresses.push_back(
          Multiaddress::create("/ip4/127.0.0.1/tcp/1").value());
      multiaddresses.push_back(
          Multiaddress::create("/ip4/127.0.0.1/tcp/2").value());

      Bytes peer{"0102"_unhex};

      EXPECT_OUTCOME_TRUE(
          info,
          makeMinerInfo(actor_version,
                        Address::makeFromId(1),
                        Address::makeFromId(2),
                        {},
                        peer,
                        multiaddresses,
                        RegisteredSealProof::kStackedDrg32GiBV1_1,
                        RegisteredPoStProof::kUndefined));

      EXPECT_OUTCOME_TRUE_1(state.miner_info.set(info));

      VestingFunds vesting_funds;
      EXPECT_OUTCOME_TRUE_1(state.vesting_funds.set(vesting_funds));

      RleBitset allocated_sectors;
      EXPECT_OUTCOME_TRUE_1(state.allocated_sectors.set(allocated_sectors));

      EXPECT_OUTCOME_TRUE(empty_amt_cid,
                          state.precommitted_setctors_expiry.amt.flush());
      EXPECT_OUTCOME_TRUE(deadlines,
                          makeEmptyDeadlines(runtime, empty_amt_cid));
      EXPECT_OUTCOME_TRUE_1(state.deadlines.set(deadlines));
    }

    Universal<SectorOnChainInfo> createSectorOnChainInfo(
        SectorNumber sector_n,
        const CID &sealed,
        const DealWeight &weight,
        ChainEpoch activation) const {
      Universal<SectorOnChainInfo> sector{actor_version};
      sector->sector = sector_n;
      sector->seal_proof = RegisteredSealProof::kStackedDrg32GiBV1_1;
      sector->sealed_cid = sealed;
      sector->deals = {};
      sector->activation_epoch = activation;
      sector->expiration = sector_expiration;
      sector->deal_weight = weight;
      sector->verified_deal_weight = weight;
      sector->init_pledge = 0;
      sector->expected_day_reward = 0;
      sector->expected_storage_pledge = 0;
      return sector;
    }

    SectorPreCommitInfo createSectorPreCommitInfo(SectorNumber sector_n,
                                                  const CID &sealed) const {
      return SectorPreCommitInfo{
          .registered_proof = RegisteredSealProof::kStackedDrg32GiBV1_1,
          .sector = sector_n,
          .sealed_cid = sealed,
          .seal_epoch = sector_seal_rand_epoch_value,
          .deal_ids = {},
          .expiration = sector_expiration,
          .replace_capacity = false,
          .replace_deadline = 0,
          .replace_partition = 0,
          .replace_sector = 0};
    }

    SectorPreCommitOnChainInfo createSectorPreCommitOnChainInfo(
        SectorNumber sector_n,
        const CID &sealed,
        const TokenAmount &deposit,
        ChainEpoch epoch) const {
      return SectorPreCommitOnChainInfo{
          .info = createSectorPreCommitInfo(sector_n, sealed),
          .precommit_deposit = deposit,
          .precommit_epoch = epoch,
          .deal_weight = 0,
          .verified_deal_weight = 0};
    }

    std::vector<VestingTestCase> getVestingTestCases() const {
      std::vector<VestingTestCase> test_cases;

      test_cases.push_back({.vspec = {.initial_delay = 0,
                                      .vest_period = 1,
                                      .step_duration = 1,
                                      .quantization = 1},
                            .period_start = 0,
                            .vepochs = {0, 0, 100, 0}});

      test_cases.push_back({.vspec = {.initial_delay = 0,
                                      .vest_period = 2,
                                      .step_duration = 1,
                                      .quantization = 1},
                            .period_start = 0,
                            .vepochs = {0, 0, 50, 50, 0}});

      test_cases.push_back({.vspec = {.initial_delay = 0,
                                      .vest_period = 2,
                                      .step_duration = 1,
                                      .quantization = 2},
                            .period_start = 0,
                            .vepochs = {0, 0, 0, 100, 0}});

      test_cases.push_back({.vspec = {.initial_delay = 0,
                                      .vest_period = 3,
                                      .step_duration = 1,
                                      .quantization = 1},
                            .period_start = 0,
                            .vepochs = {0, 0, 33, 33, 34, 0}});

      test_cases.push_back({.vspec = {.initial_delay = 0,
                                      .vest_period = 3,
                                      .step_duration = 1,
                                      .quantization = 2},
                            .period_start = 0,
                            .vepochs = {0, 0, 0, 66, 0, 34, 0}});

      test_cases.push_back({.vspec = {.initial_delay = 0,
                                      .vest_period = 2,
                                      .step_duration = 2,
                                      .quantization = 1},
                            .period_start = 0,
                            .vepochs = {0, 0, 0, 100, 0}});

      test_cases.push_back({.vspec = {.initial_delay = 0,
                                      .vest_period = 5,
                                      .step_duration = 2,
                                      .quantization = 1},
                            .period_start = 0,
                            .vepochs = {0, 0, 0, 40, 0, 40, 0, 20, 0}});

      test_cases.push_back({.vspec = {.initial_delay = 1,
                                      .vest_period = 5,
                                      .step_duration = 2,
                                      .quantization = 1},
                            .period_start = 0,
                            .vepochs = {0, 0, 0, 0, 40, 0, 40, 0, 20, 0}});

      test_cases.push_back({.vspec = {.initial_delay = 0,
                                      .vest_period = 5,
                                      .step_duration = 2,
                                      .quantization = 2},
                            .period_start = 0,
                            .vepochs = {0, 0, 0, 40, 0, 40, 0, 20, 0}});

      test_cases.push_back({.vspec = {.initial_delay = 0,
                                      .vest_period = 5,
                                      .step_duration = 3,
                                      .quantization = 1},
                            .period_start = 0,
                            .vepochs = {0, 0, 0, 0, 60, 0, 0, 40, 0}});

      test_cases.push_back({.vspec = {.initial_delay = 0,
                                      .vest_period = 5,
                                      .step_duration = 3,
                                      .quantization = 2},
                            .period_start = 0,
                            .vepochs = {0, 0, 0, 0, 0, 80, 0, 20, 0}});

      test_cases.push_back({.vspec = {.initial_delay = 0,
                                      .vest_period = 5,
                                      .step_duration = 6,
                                      .quantization = 1},
                            .period_start = 0,
                            .vepochs = {0, 0, 0, 0, 0, 0, 0, 100, 0}});

      test_cases.push_back(
          {.vspec = {.initial_delay = 5,
                     .vest_period = 5,
                     .step_duration = 1,
                     .quantization = 1},
           .period_start = 0,
           .vepochs = {0, 0, 0, 0, 0, 0, 0, 20, 20, 20, 20, 20, 0}});

      test_cases.push_back(
          {.vspec = {.initial_delay = 0,
                     .vest_period = 10,
                     .step_duration = 2,
                     .quantization = 2},
           .period_start = 0,
           .vepochs = {0, 0, 0, 20, 0, 20, 0, 20, 0, 20, 0, 20}});

      test_cases.push_back(
          {.vspec = {.initial_delay = 0,
                     .vest_period = 10,
                     .step_duration = 2,
                     .quantization = 2},
           .period_start = 1,
           .vepochs = {0, 0, 0, 20, 0, 20, 0, 20, 0, 20, 0, 20}});

      test_cases.push_back(
          {.vspec = {.initial_delay = 0,
                     .vest_period = 10,
                     .step_duration = 2,
                     .quantization = 2},
           .period_start = 55,
           .vepochs = {0, 0, 0, 20, 0, 20, 0, 20, 0, 20, 0, 20}});

      test_cases.push_back({.vspec = {.initial_delay = 0,
                                      .vest_period = 10,
                                      .step_duration = 1,
                                      .quantization = 5},
                            .period_start = 0,
                            .vepochs = {0, 0, 0, 0, 0, 0, 50, 0, 0, 0, 0, 50}});

      return test_cases;
    }

    MockRuntime runtime;
    std::shared_ptr<InMemoryDatastore> ipld{
        std::make_shared<InMemoryDatastore>()};
    ActorVersion actor_version;

    MinerActorState state;

    ChainEpoch epoch;
    SectorNumber sector_num;
    const ChainEpoch sector_seal_rand_epoch_value{1};
    const ChainEpoch sector_expiration{1};
    ChainEpoch vest_start;
    TokenAmount vest_sum;
  };

  TEST_F(MinerActorStateTestV2, PrecommittedSectorsStorePutGetAndDelete) {
    const auto pc1 =
        createSectorPreCommitOnChainInfo(sector_num, "010001020001"_cid, 1, 1);
    EXPECT_OUTCOME_TRUE_1(state.precommitted_sectors.set(pc1.info.sector, pc1));
    EXPECT_OUTCOME_EQ(state.precommitted_sectors.get(sector_num), pc1);

    const auto pc2 =
        createSectorPreCommitOnChainInfo(sector_num, "010001020002"_cid, 1, 1);
    EXPECT_OUTCOME_TRUE_1(state.precommitted_sectors.set(pc1.info.sector, pc2));
    EXPECT_OUTCOME_EQ(state.precommitted_sectors.get(sector_num), pc2);

    EXPECT_OUTCOME_TRUE_1(state.deletePrecommittedSectors({sector_num}));
    EXPECT_OUTCOME_EQ(state.precommitted_sectors.has(sector_num), false);
  }

  TEST_F(MinerActorStateTestV2,
         PrecommittedSectorsStoreDeleteNonexistentValueReturnsAnError) {
    const auto result = state.deletePrecommittedSectors({sector_num});
    EXPECT_EQ(result.error().message(), "Not found");
  }

  TEST_F(MinerActorStateTestV2, SectorsStorePutGetAndDelete) {
    const auto sector_info1 =
        createSectorOnChainInfo(sector_num, "010001020001"_cid, 1, 1);
    const auto sector_info2 =
        createSectorOnChainInfo(sector_num, "010001020002"_cid, 2, 2);

    EXPECT_OUTCOME_TRUE_1(state.sectors.store({sector_info1}));
    EXPECT_OUTCOME_EQ(state.sectors.sectors.get(sector_num), sector_info1);

    EXPECT_OUTCOME_TRUE_1(state.sectors.store({sector_info2}));
    EXPECT_OUTCOME_EQ(state.sectors.sectors.get(sector_num), sector_info2);

    EXPECT_OUTCOME_TRUE_1(state.sectors.sectors.remove(sector_num));
    EXPECT_OUTCOME_EQ(state.sectors.sectors.has(sector_num), false);
  }

  TEST_F(MinerActorStateTestV2, TestVesting) {
    ChainEpoch vest_start_delay = 10;

    for (const auto &test_case : getVestingTestCases()) {
      state.proving_period_start = test_case.period_start;
      const auto start = test_case.period_start + vest_start_delay;

      EXPECT_OUTCOME_TRUE_1(
          state.addLockedFunds(start, vest_sum, test_case.vspec));
      EXPECT_EQ(state.locked_funds, vest_sum);

      int64_t total_vested = 0;
      for (size_t i = 0; i < test_case.vepochs.size(); i++) {
        const auto &vested = test_case.vepochs[i];
        EXPECT_OUTCOME_EQ(state.unlockVestedFunds(start + i), vested);
        total_vested += vested;
        EXPECT_EQ(state.locked_funds, vest_sum - total_vested);
      }

      EXPECT_EQ(vest_sum, total_vested);
      EXPECT_OUTCOME_TRUE(funds, state.vesting_funds.get());
      EXPECT_TRUE(funds.funds.empty());
      EXPECT_EQ(state.locked_funds, 0);
    }
  }

  TEST_F(MinerActorStateTestV2, LockedfundsIncreasesWithSequentialCalls) {
    const VestSpec vspec{.initial_delay = 0,
                         .vest_period = 1,
                         .step_duration = 1,
                         .quantization = 1};

    EXPECT_OUTCOME_TRUE_1(state.addLockedFunds(vest_start, vest_sum, vspec));
    EXPECT_EQ(state.locked_funds, vest_sum);

    EXPECT_OUTCOME_TRUE_1(state.addLockedFunds(vest_start, vest_sum, vspec));
    EXPECT_EQ(state.locked_funds, 2 * vest_sum);
  }

  TEST_F(MinerActorStateTestV2,
         VestsWhenQuantizeStepDurationAndVestingPeriodAreCoprime) {
    const VestSpec vspec{.initial_delay = 0,
                         .vest_period = 27,
                         .step_duration = 5,
                         .quantization = 7};

    EXPECT_OUTCOME_TRUE_1(state.addLockedFunds(vest_start, vest_sum, vspec));
    EXPECT_EQ(state.locked_funds, vest_sum);

    TokenAmount total_vested = 0;
    for (auto epoch = vest_start; epoch <= 43; epoch++) {
      EXPECT_OUTCOME_TRUE(amount_vested, state.unlockVestedFunds(epoch));

      switch (epoch) {
        case 22:
          EXPECT_EQ(amount_vested, 40);
          total_vested += amount_vested;
          break;
        case 29:
          EXPECT_EQ(amount_vested, 26);
          total_vested += amount_vested;
          break;
        case 36:
          EXPECT_EQ(amount_vested, 26);
          total_vested += amount_vested;
          break;
        case 43:
          EXPECT_EQ(amount_vested, 8);
          total_vested += amount_vested;
          break;
        default:
          EXPECT_EQ(amount_vested, 0);
          break;
      }
    }

    EXPECT_EQ(total_vested, vest_sum);
    EXPECT_EQ(state.locked_funds, 0);
    EXPECT_OUTCOME_TRUE(funds, state.vesting_funds.get());
    EXPECT_TRUE(funds.funds.empty());
  }

  TEST_F(MinerActorStateTestV2,
         UnlockUnvestedFundsLeavingBucketWithNonZeroTokens) {
    vest_start = 100;
    const VestSpec vspec{.initial_delay = 0,
                         .vest_period = 5,
                         .step_duration = 1,
                         .quantization = 1};

    EXPECT_OUTCOME_TRUE_1(state.addLockedFunds(vest_start, vest_sum, vspec));

    EXPECT_OUTCOME_EQ(state.unlockUnvestedFunds(vest_start, 39), 39);

    EXPECT_OUTCOME_EQ(state.unlockVestedFunds(vest_start), 0);
    EXPECT_OUTCOME_EQ(state.unlockVestedFunds(vest_start + 1), 0);

    // expected to be zero due to unlocking of Unvested funds
    EXPECT_OUTCOME_EQ(state.unlockVestedFunds(vest_start + 2), 0);
    // expected to be non-zero due to unlocking of Unvested funds
    EXPECT_OUTCOME_EQ(state.unlockVestedFunds(vest_start + 3), 1);

    EXPECT_OUTCOME_EQ(state.unlockVestedFunds(vest_start + 4), 20);
    EXPECT_OUTCOME_EQ(state.unlockVestedFunds(vest_start + 5), 20);
    EXPECT_OUTCOME_EQ(state.unlockVestedFunds(vest_start + 6), 20);

    EXPECT_OUTCOME_EQ(state.unlockVestedFunds(vest_start + 7), 0);

    EXPECT_EQ(state.locked_funds, 0);
    EXPECT_OUTCOME_TRUE(funds, state.vesting_funds.get());
    EXPECT_TRUE(funds.funds.empty());
  }

  TEST_F(MinerActorStateTestV2,
         UnlockUnvestedFundsLeavingBucketWithZeroTokens) {
    vest_start = 100;
    const VestSpec vspec{.initial_delay = 0,
                         .vest_period = 5,
                         .step_duration = 1,
                         .quantization = 1};

    EXPECT_OUTCOME_TRUE_1(state.addLockedFunds(vest_start, vest_sum, vspec));

    EXPECT_OUTCOME_EQ(state.unlockUnvestedFunds(vest_start, 40), 40);

    EXPECT_OUTCOME_EQ(state.unlockVestedFunds(vest_start), 0);
    EXPECT_OUTCOME_EQ(state.unlockVestedFunds(vest_start + 1), 0);

    // expected to be zero due to unlocking of Unvested funds
    EXPECT_OUTCOME_EQ(state.unlockVestedFunds(vest_start + 2), 0);
    EXPECT_OUTCOME_EQ(state.unlockVestedFunds(vest_start + 3), 0);

    EXPECT_OUTCOME_EQ(state.unlockVestedFunds(vest_start + 4), 20);
    EXPECT_OUTCOME_EQ(state.unlockVestedFunds(vest_start + 5), 20);
    EXPECT_OUTCOME_EQ(state.unlockVestedFunds(vest_start + 6), 20);

    EXPECT_OUTCOME_EQ(state.unlockVestedFunds(vest_start + 7), 0);

    EXPECT_EQ(state.locked_funds, 0);
    EXPECT_OUTCOME_TRUE(funds, state.vesting_funds.get());
    EXPECT_TRUE(funds.funds.empty());
  }

  TEST_F(MinerActorStateTestV2, UnlockAllUnvestedFunds) {
    const VestSpec vspec{.initial_delay = 0,
                         .vest_period = 5,
                         .step_duration = 1,
                         .quantization = 1};

    EXPECT_OUTCOME_TRUE_1(state.addLockedFunds(vest_start, vest_sum, vspec));
    EXPECT_OUTCOME_EQ(state.unlockUnvestedFunds(vest_start, vest_sum),
                      vest_sum);

    EXPECT_EQ(state.locked_funds, 0);
    EXPECT_OUTCOME_TRUE(funds, state.vesting_funds.get());
    EXPECT_TRUE(funds.funds.empty());
  }

  TEST_F(MinerActorStateTestV2,
         UnlockUnvestedFundsValueGreaterThanLockedfunds) {
    const VestSpec vspec{.initial_delay = 0,
                         .vest_period = 1,
                         .step_duration = 1,
                         .quantization = 1};

    EXPECT_OUTCOME_TRUE_1(state.addLockedFunds(vest_start, vest_sum, vspec));
    EXPECT_OUTCOME_EQ(state.unlockUnvestedFunds(vest_start, 200), vest_sum);

    EXPECT_EQ(state.locked_funds, 0);
    EXPECT_OUTCOME_TRUE(funds, state.vesting_funds.get());
    EXPECT_TRUE(funds.funds.empty());
  }

  TEST_F(MinerActorStateTestV2,
         UnlockUnvestedFundsWhenThereAreVestedFundsInTheTable) {
    const VestSpec vspec{.initial_delay = 0,
                         .vest_period = 50,
                         .step_duration = 1,
                         .quantization = 1};

    EXPECT_OUTCOME_TRUE_1(state.addLockedFunds(vest_start, vest_sum, vspec));

    const ChainEpoch new_epoch = 30;
    const TokenAmount target = 60;
    const TokenAmount remaining = vest_sum - target;

    EXPECT_OUTCOME_EQ(state.unlockUnvestedFunds(new_epoch, target), target);
    EXPECT_EQ(state.locked_funds, remaining);

    EXPECT_OUTCOME_TRUE(funds, state.vesting_funds.get());
    epoch = 11;

    for (const auto &vf : funds.funds) {
      EXPECT_EQ(vf.epoch, epoch);
      epoch++;
      if (epoch == 30) {
        break;
      }
    }
  }

  TEST_F(MinerActorStateTestV2, SuccessfullyAddAProofToPreCommitExpiryQueue) {
    EXPECT_OUTCOME_TRUE_1(state.addPreCommitExpiry(epoch, sector_num));

    const auto quant = state.quantSpecEveryDeadline();
    BitfieldQueue<kPrecommitExpiryBitwidth> queue{
        .queue = state.precommitted_setctors_expiry, .quant = quant};

    EXPECT_OUTCOME_EQ(queue.queue.size(), 1);
    const auto q_epoch = quant.quantizeUp(epoch);
    EXPECT_OUTCOME_TRUE(buf, queue.queue.get(q_epoch));
    EXPECT_EQ(buf.size(), 1);
    EXPECT_TRUE(buf.has(sector_num));
  }

  TEST_F(MinerActorStateTestV2, AssignSectorsToDeadlines) {
    EXPECT_OUTCOME_TRUE(partition_sectors,
                        getSealProofWindowPoStPartitionSectors(
                            RegisteredSealProof::kStackedDrg32GiBV1_1));
    EXPECT_OUTCOME_TRUE(
        ssize, getSectorSize(RegisteredSealProof::kStackedDrg32GiBV1_1));
    const auto open_deadlines = kWPoStPeriodDeadlines - 2;
    const uint64_t partitions_per_deadline = 3;
    const auto no_sectors =
        partition_sectors * open_deadlines * partitions_per_deadline;

    std::vector<Universal<SectorOnChainInfo>> sector_infos;
    for (size_t i = 0; i < no_sectors; i++) {
      sector_infos.push_back(
          createSectorOnChainInfo(i, "010001020001"_cid, 1, 0));
    }

    ExpectedDeadline dl_state_origin;
    dl_state_origin.ssize = ssize;
    dl_state_origin.partition_size = partition_sectors;
    dl_state_origin.sectors = sector_infos;

    EXPECT_OUTCOME_TRUE(
        new_power,
        state.assignSectorsToDeadlines(
            runtime, 0, sector_infos, partition_sectors, ssize));
    EXPECT_TRUE(new_power.isZero());

    Sectors sectors;
    cbor_blake::cbLoadT(ipld, sectors);
    EXPECT_OUTCOME_TRUE_1(sectors.store(sector_infos));

    EXPECT_OUTCOME_TRUE(dls, state.deadlines.get());

    for (uint64_t dl_id = 0; dl_id < dls.due.size(); dl_id++) {
      EXPECT_OUTCOME_TRUE(deadline, dls.loadDeadline(dl_id));
      const auto quant = state.quantSpecForDeadline(dl_id);
      auto dl_state = dl_state_origin;
      dl_state.quant = quant;

      // deadlines 0 & 1 are closed for assignment right now
      if (dl_id < 2) {
        dl_state.assertDeadline(deadline);
        continue;
      }

      std::vector<RleBitset> partitions;
      std::vector<PoStPartition> post_partitions;
      for (uint64_t i = 0; i < partitions_per_deadline; i++) {
        const auto start =
            ((i * open_deadlines) + (dl_id - 2)) * partition_sectors;
        RleBitset buf;
        for (uint64_t j = 0; j < partition_sectors; j++) {
          buf.insert(start + j);
        }
        partitions.push_back(buf);
        post_partitions.push_back({.index = i, .skipped = {}});
      }

      RleBitset all_sectors;
      all_sectors += partitions;

      dl_state.partition_sectors = partitions;
      dl_state.unproven = all_sectors;
      dl_state.assertDeadline(deadline);

      EXPECT_OUTCOME_TRUE(result,
                          deadline->recordProvenSectors(
                              sectors, ssize, quant, 0, post_partitions));

      EXPECT_EQ(result.sectors, all_sectors);
      EXPECT_TRUE(result.ignored_sectors.empty());
      EXPECT_TRUE(result.new_faulty_power.isZero());
      EXPECT_EQ(
          result.power_delta,
          powerForSectors(ssize, selectSectorsTest(sector_infos, all_sectors)));
      EXPECT_TRUE(result.recovered_power.isZero());
      EXPECT_TRUE(result.retracted_recovery_power.isZero());
    }
  }

  TEST_F(MinerActorStateTestV2, CantAllocateTheSameSectorNumberTwice) {
    EXPECT_OUTCOME_TRUE_1(state.allocateSectorNumber(sector_num));
    EXPECT_OUTCOME_ERROR(VMExitCode::kErrIllegalArgument,
                         state.allocateSectorNumber(sector_num));
  }

  TEST_F(MinerActorStateTestV2, CanMaskSectorNumbers) {
    EXPECT_OUTCOME_TRUE_1(state.allocateSectorNumber(sector_num));
    EXPECT_OUTCOME_TRUE_1(state.maskSectorNumbers({0, 1, 2, 3}));

    EXPECT_OUTCOME_ERROR(VMExitCode::kErrIllegalArgument,
                         state.allocateSectorNumber(3));
    EXPECT_OUTCOME_TRUE_1(state.allocateSectorNumber(4));
  }

  TEST_F(MinerActorStateTestV2, CantAllocateOrMaskOutOfRange) {
    EXPECT_OUTCOME_ERROR(VMExitCode::kErrIllegalArgument,
                         state.allocateSectorNumber(kMaxSectorNumber + 1));

    EXPECT_OUTCOME_ERROR(VMExitCode::kErrIllegalArgument,
                         state.maskSectorNumbers({99, kMaxSectorNumber + 1}));
  }

  TEST_F(MinerActorStateTestV2, CanAllocateInRange) {
    EXPECT_OUTCOME_TRUE_1(state.allocateSectorNumber(kMaxSectorNumber));
    EXPECT_OUTCOME_TRUE_1(state.maskSectorNumbers({99, kMaxSectorNumber}));
  }

  TEST_F(MinerActorStateTestV2, RepayDebtInPriorityOrder) {
    TokenAmount curr_balance = 300;
    TokenAmount fee = 1000;

    EXPECT_OUTCOME_TRUE_1(state.applyPenalty(fee));
    EXPECT_EQ(state.fee_debt, fee);

    EXPECT_OUTCOME_TRUE(result,
                        state.repayPartialDebtInPriorityOrder(0, curr_balance));
    const auto &[penalty_from_vesting, penalty_from_balance] = result;
    EXPECT_EQ(penalty_from_vesting, 0);
    EXPECT_EQ(penalty_from_balance, curr_balance);

    TokenAmount expected_debt = -(curr_balance - fee);
    EXPECT_EQ(state.fee_debt, expected_debt);

    curr_balance = 0;
    fee = 2050;

    EXPECT_OUTCOME_TRUE_1(state.applyPenalty(fee));

    EXPECT_OUTCOME_TRUE_1(
        state.repayPartialDebtInPriorityOrder(33, curr_balance));

    expected_debt += fee;
    EXPECT_EQ(state.fee_debt, expected_debt);
  }

}  // namespace fc::vm::actor::builtin::v2::miner
