/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/states/miner/miner_actor_state.hpp"
#include "vm/actor/builtin/v2/miner/miner_actor.hpp"

#include "primitives/address/address.hpp"
#include "primitives/cid/comm_cid.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"
#include "testutil/resources/parse.hpp"
#include "testutil/resources/resources.hpp"
#include "testutil/vm/actor/builtin/miner/miner_actor_test_fixture.hpp"
#include "vm/actor/builtin/types/miner/policy.hpp"
#include "vm/actor/builtin/v2/market/market_actor.hpp"
#include "vm/actor/builtin/v2/reward/reward_actor.hpp"
#include "vm/actor/builtin/v2/storage_power/storage_power_actor.hpp"
#include "vm/actor/codes.hpp"

namespace fc::vm::actor::builtin::v2::miner {
  using common::smoothing::FilterEstimate;
  using crypto::randomness::Randomness;
  using primitives::kChainEpochUndefined;
  using primitives::address::decodeFromString;
  using primitives::cid::replicaCommitmentV1ToCID;
  using primitives::sector::RegisteredSealProof;
  using states::MinerActorStatePtr;
  using testing::_;
  using testing::Return;
  using testutil::vm::actor::builtin::miner::MinerActorTestFixture;
  using types::Universal;
  using namespace types::miner;

  class MinerActorTest : public MinerActorTestFixture {
   public:
    void SetUp() override {
      MinerActorTestFixture::SetUp();
      actor_version = ActorVersion::kVersion2;
      ipld->actor_version = actor_version;
      state = MinerActorStatePtr{actor_version};
      anyCodeIdAddressIs(kAccountCodeId);
      cbor_blake::cbLoadT(ipld, state);

      currentEpochIs(kUpgradeCalicoHeight + 1);
    }

    /**
     * Creates simple valid construct parameters
     */
    Construct::Params makeConstructParams() {
      return {.owner = owner,
              .worker = worker,
              .control_addresses = {},
              .seal_proof_type = RegisteredSealProof::kStackedDrg32GiBV1_1,
              .peer_id = {},
              .multiaddresses = {}};
    }

    void expectEnrollCronEvent(const ChainEpoch &proving_period_start) {
      const auto deadline_index =
          (current_epoch - proving_period_start) / kWPoStChallengeWindow;
      const auto first_deadline_close =
          proving_period_start + (1 + deadline_index) * kWPoStChallengeWindow;
      CronEventPayload payload{CronEventType::kProvingDeadline};
      EXPECT_OUTCOME_TRUE(encoded_payload, codec::cbor::encode(payload));
      runtime.expectSendM<storage_power::EnrollCronEvent>(
          kStoragePowerAddress,
          {.event_epoch = first_deadline_close - 1, .payload = encoded_payload},
          0,
          {});
    }

    void expectThisEpochReward(const FilterEstimate &reward_smoothed,
                               const StoragePower &baseline_power) {
      runtime.expectSendM<reward::ThisEpochReward>(
          kRewardAddress, {}, 0, {reward_smoothed, baseline_power});
    }

    void expectCurrentTotalPower(const StoragePower &raw,
                                 const StoragePower &qa,
                                 const TokenAmount &pledge_collateral,
                                 const FilterEstimate &qa_power_smoothed) {
      runtime.expectSendM<storage_power::CurrentTotalPower>(
          kStoragePowerAddress,
          {},
          0,
          {raw, qa, pledge_collateral, qa_power_smoothed});
    }

    void expectDealWeight(const std::vector<DealId> &deals,
                          ChainEpoch sector_start,
                          ChainEpoch sector_expiry,
                          const DealWeight &deal_weight,
                          const DealWeight &verified_deal_weight,
                          uint64_t deal_space) {
      runtime.expectSendM<market::VerifyDealsForActivation>(
          kStorageMarketAddress,
          {deals, sector_expiry, sector_start},
          0,
          {deal_weight, verified_deal_weight, deal_space});
    }

    const Blob<48> bls_pubkey =
        "1234567890123456789012345678901234567890"
        "1234567890123456789012345678901234567890"
        "1122334455667788"_blob48;
  };

  /**
   * Simple construction
   * @given vm
   * @when construct method called
   * @then empty miner actor created
   */
  TEST_F(MinerActorTest, SimpleConstruct) {
    callerIs(kInitAddress);

    expectAccountV2PubkeyAddressSend(worker, bls_pubkey);

    EXPECT_CALL(runtime, getCurrentReceiver())
        .WillRepeatedly(Return(Address::makeFromId(1000)));

    // This is just set from running the code.
    const ChainEpoch proving_period_start{262675};
    const auto deadline_index =
        (current_epoch - proving_period_start) / kWPoStChallengeWindow;
    expectEnrollCronEvent(proving_period_start);

    Construct::Params params = makeConstructParams();
    params.worker = worker;
    EXPECT_OUTCOME_TRUE_1(Construct::call(runtime, params));

    EXPECT_OUTCOME_TRUE(miner_info, state->getInfo());
    EXPECT_EQ(miner_info->owner, params.owner);
    EXPECT_EQ(miner_info->worker, params.worker);
    EXPECT_EQ(miner_info->control, params.control_addresses);
    EXPECT_EQ(miner_info->peer_id, params.peer_id);
    EXPECT_EQ(miner_info->multiaddrs, params.multiaddresses);
    EXPECT_EQ(static_cast<RegisteredSealProof>(miner_info->seal_proof_type),
              RegisteredSealProof::kStackedDrg32GiBV1_1);
    EXPECT_EQ(miner_info->sector_size, BigInt{1} << 35);
    EXPECT_EQ(miner_info->window_post_partition_sectors, 2349);
    EXPECT_EQ(miner_info->consensus_fault_elapsed, kChainEpochUndefined);
    EXPECT_EQ(miner_info->pending_owner_address, boost::none);

    EXPECT_EQ(state->precommit_deposit, 0);
    EXPECT_EQ(state->locked_funds, 0);
    EXPECT_EQ(state->proving_period_start, proving_period_start);
    EXPECT_EQ(state->current_deadline, deadline_index);

    EXPECT_OUTCOME_TRUE(deadlines, state->deadlines.get());
    EXPECT_EQ(deadlines.due.size(), kWPoStPeriodDeadlines);

    for (const auto &deadline_cid : deadlines.due) {
      EXPECT_OUTCOME_TRUE(deadline, deadline_cid.get());
      EXPECT_OUTCOME_EQ(deadline->partitions.size(), 0);
      EXPECT_OUTCOME_EQ(deadline->expirations_epochs.size(), 0);
      EXPECT_TRUE(deadline->partitions_posted.empty());
      EXPECT_TRUE(deadline->early_terminations.empty());
      EXPECT_EQ(deadline->live_sectors, 0);
      EXPECT_EQ(deadline->total_sectors, 0);
      EXPECT_EQ(deadline->faulty_power.raw, 0);
      EXPECT_EQ(deadline->faulty_power.qa, 0);
    }
  }

  /**
   * @given vm and control addresses are resolvable
   * @when miner is constructed
   * @then control addresses are resolved
   */
  TEST_F(MinerActorTest, ConstructResolvedControl) {
    callerIs(kInitAddress);

    expectAccountV2PubkeyAddressSend(worker, bls_pubkey);

    std::vector<Address> control_addresses;
    const Address control1 = Address::makeFromId(501);
    const Address controlId1 = Address::makeFromId(555);
    control_addresses.emplace_back(control1);
    resolveAddressAs(control1, controlId1);

    const Address control2 = Address::makeFromId(502);
    const Address controlId2 = Address::makeFromId(655);
    control_addresses.emplace_back(control2);
    resolveAddressAs(control2, controlId2);

    EXPECT_CALL(runtime, getCurrentReceiver())
        .WillRepeatedly(Return(Address::makeFromId(1000)));

    // This is just set from running the code.
    const ChainEpoch proving_period_start{262675};
    expectEnrollCronEvent(proving_period_start);

    Construct::Params params = makeConstructParams();
    params.worker = worker;
    params.control_addresses = control_addresses;
    EXPECT_OUTCOME_TRUE_1(Construct::call(runtime, params));

    EXPECT_OUTCOME_TRUE(miner_info, state->getInfo());
    EXPECT_EQ(miner_info->control.size(), 2);
    EXPECT_EQ(miner_info->control[0], controlId1);
    EXPECT_EQ(miner_info->control[1], controlId2);
  }

  /**
   * @given vm and control addresses are not id addresses
   * @when miner constructor called
   * @then error returned
   */
  TEST_F(MinerActorTest, ConstructControlNotId) {
    callerIs(kInitAddress);

    expectAccountV2PubkeyAddressSend(worker, bls_pubkey);

    std::vector<Address> control_addresses;
    control_addresses.emplace_back(control);
    addressCodeIdIs(control, kCronCodeId);

    Construct::Params params = makeConstructParams();
    params.worker = worker;
    params.control_addresses = control_addresses;
    EXPECT_OUTCOME_ERROR(asAbort(VMExitCode::kErrIllegalArgument),
                         Construct::call(runtime, params));
  }

  /**
   * @given PeerId too large
   * @when miner constructor called
   * @then error returned
   */
  TEST_F(MinerActorTest, ConstructTooLargePeerId) {
    callerIs(kInitAddress);
    const Bytes wrong_peer_id(kMaxPeerIDLength + 1, 'x');

    Construct::Params params = makeConstructParams();
    params.peer_id = wrong_peer_id;
    EXPECT_OUTCOME_ERROR(asAbort(VMExitCode::kErrIllegalArgument),
                         Construct::call(runtime, params));
  }

  /**
   * @given control addresses exceed limit
   * @when miner constructor called
   * @then error returned
   */
  TEST_F(MinerActorTest, ConstructControlAddressesExceedLimit) {
    callerIs(kInitAddress);
    std::vector<Address> control_addresses(kMaxControlAddresses + 1, control);

    Construct::Params params = makeConstructParams();
    params.control_addresses = control_addresses;
    EXPECT_OUTCOME_ERROR(asAbort(VMExitCode::kErrIllegalArgument),
                         Construct::call(runtime, params));
  }

  /**
   * @given multiaddresses size too large
   * @when miner constructor called
   * @then error returned
   */
  TEST_F(MinerActorTest, ConstructMultiaddressesTooLarge) {
    callerIs(kInitAddress);
    Multiaddress multiaddress =
        Multiaddress::create("/ip4/127.0.0.1/tcp/111").value();

    Construct::Params params = makeConstructParams();
    params.multiaddresses = std::vector<Multiaddress>(1000, multiaddress);
    EXPECT_OUTCOME_ERROR(asAbort(VMExitCode::kErrIllegalArgument),
                         Construct::call(runtime, params));
  }

  /**
   * Paramtrized parameters for testing construction with different network
   * versions and supported proof types
   */
  using ConstructParams = std::pair<NetworkVersion, RegisteredSealProof>;

  /**
   * Successful construction tests
   */
  struct ConstructSuccesssMinerActorTest
      : public MinerActorTest,
        public ::testing::WithParamInterface<ConstructParams> {};

  /**
   * @given Successful construction with given network version and proof type
   * @when miner constructor called
   * @then success
   */
  TEST_P(ConstructSuccesssMinerActorTest, ConstructParametrizedNVSuccess) {
    callerIs(kInitAddress);

    const auto &[netwrok_version, seal_proof_type] = GetParam();

    EXPECT_CALL(runtime, getCurrentReceiver())
        .WillRepeatedly(Return(Address::makeFromId(1000)));

    expectAccountV2PubkeyAddressSend(worker, bls_pubkey);

    // This is just set from running the code.
    const ChainEpoch proving_period_start{262675};
    expectEnrollCronEvent(proving_period_start);

    Construct::Params params = makeConstructParams();
    params.worker = worker;
    params.seal_proof_type = seal_proof_type;
    EXPECT_OUTCOME_TRUE_1(Construct::call(runtime, params));
  }

  INSTANTIATE_TEST_SUITE_P(
      ConstructSuccesssMinerActorTestCases,
      ConstructSuccesssMinerActorTest,
      ::testing::Values(
          // version < 7 accepts only StackedDrg32GiBV1
          ConstructParams{NetworkVersion::kVersion6,
                          RegisteredSealProof::kStackedDrg32GiBV1},
          // version 7 accepts both StackedDrg32GiBV1 and StackedDrg32GiBV1_1
          ConstructParams{NetworkVersion::kVersion7,
                          RegisteredSealProof::kStackedDrg32GiBV1},
          ConstructParams{NetworkVersion::kVersion7,
                          RegisteredSealProof::kStackedDrg32GiBV1_1},
          // version > 7 accepts only StackedDrg32GiBV1_1
          ConstructParams{NetworkVersion::kVersion8,
                          RegisteredSealProof::kStackedDrg32GiBV1_1}));

  /**
   * Failed construction tests
   */
  struct ConstructFailureMinerActorTest
      : public MinerActorTest,
        public ::testing::WithParamInterface<ConstructParams> {};

  INSTANTIATE_TEST_SUITE_P(
      ConstructFailureMinerActorTestCases,
      ConstructFailureMinerActorTest,
      ::testing::Values(
          // version < 7 accepts only StackedDrg32GiBV1
          ConstructParams{NetworkVersion::kVersion6,
                          RegisteredSealProof::kStackedDrg32GiBV1_1},
          // version > 7 accepts only StackedDrg32GiBV1_1
          ConstructParams{NetworkVersion::kVersion8,
                          RegisteredSealProof::kStackedDrg32GiBV1}));

  /**
   * @given Construction with wrong network version and proof type
   * @when miner constructor called
   * @then error returned
   */
  TEST_P(ConstructFailureMinerActorTest, ConstructParametrizedNVFailure) {
    callerIs(kInitAddress);

    const auto &[netwrok_version, seal_proof_type] = GetParam();

    EXPECT_CALL(runtime, getCurrentReceiver())
        .WillRepeatedly(Return(Address::makeFromId(1000)));
    EXPECT_CALL(runtime, getNetworkVersion())
        .WillRepeatedly(Return(netwrok_version));

    Construct::Params params = makeConstructParams();
    params.seal_proof_type = seal_proof_type;
    EXPECT_OUTCOME_ERROR(asAbort(VMExitCode::kErrIllegalArgument),
                         Construct::call(runtime, params));
  }

  /**
   * @given state is created
   * @when miner ControlAddresses called
   * @then properly values are returned
   */
  TEST_F(MinerActorTest, ControlAddressesSuccess) {
    initEmptyState();
    initDefaultMinerInfo();

    EXPECT_OUTCOME_TRUE(result, ControlAddresses::call(runtime, {}));

    EXPECT_EQ(result.owner, owner);
    EXPECT_EQ(result.worker, worker);
    EXPECT_EQ(result.control.size(), 1);
    EXPECT_EQ(result.control[0], control);
  }

  /**
   * @given caller is not owner
   * @when miner ChangeWorkerAddress called
   * @then kSysErrForbidden returned
   */
  TEST_F(MinerActorTest, ChangeWorkerAddressWrongCaller) {
    initEmptyState();
    initDefaultMinerInfo();

    callerIs(kInitAddress);

    const Address new_worker = Address::makeFromId(201);
    expectAccountV2PubkeyAddressSend(new_worker, bls_pubkey);

    std::vector<Address> new_control_addresses;
    const Address control1 = Address::makeFromId(701);
    const Address controlId1 = Address::makeFromId(751);
    new_control_addresses.emplace_back(control1);
    resolveAddressAs(control1, controlId1);

    const Address control2 = Address::makeFromId(702);
    const Address controlId2 = Address::makeFromId(752);
    new_control_addresses.emplace_back(control2);
    resolveAddressAs(control2, controlId2);

    EXPECT_OUTCOME_ERROR(
        asAbort(VMExitCode::kSysErrForbidden),
        ChangeWorkerAddress::call(
            runtime,
            ChangeWorkerAddress::Params{new_worker, new_control_addresses}));
  }

  /**
   * @given vm
   * @when miner ChangeWorkerAddress called
   * @then new worker is recorded to pending_worker_key
   */
  TEST_F(MinerActorTest, ChangeWorkerAddressSuccess) {
    initEmptyState();
    initDefaultMinerInfo();

    const ChainEpoch effective_epoch{current_epoch + kWorkerKeyChangeDelay};

    callerIs(owner);

    const Address new_worker = Address::makeFromId(201);
    expectAccountV2PubkeyAddressSend(new_worker, bls_pubkey);

    std::vector<Address> new_control_addresses;
    const Address control1 = Address::makeFromId(701);
    const Address controlId1 = Address::makeFromId(751);
    new_control_addresses.emplace_back(control1);
    resolveAddressAs(control1, controlId1);

    const Address control2 = Address::makeFromId(702);
    const Address controlId2 = Address::makeFromId(752);
    new_control_addresses.emplace_back(control2);
    resolveAddressAs(control2, controlId2);

    EXPECT_OUTCOME_TRUE_1(ChangeWorkerAddress::call(
        runtime,
        ChangeWorkerAddress::Params{new_worker, new_control_addresses}));

    EXPECT_OUTCOME_TRUE(miner_info, state->getInfo());
    EXPECT_EQ(miner_info->pending_worker_key.get().new_worker, new_worker);
    EXPECT_EQ(miner_info->pending_worker_key.get().effective_at,
              effective_epoch);
    EXPECT_EQ(miner_info->control.size(), 2);
    EXPECT_EQ(miner_info->control[0], controlId1);
    EXPECT_EQ(miner_info->control[1], controlId2);
  }

  /**
   * @given caller is not owner, worker or control address
   * @when miner ChangePeerId called
   * @then kSysErrForbidden returned
   */
  TEST_F(MinerActorTest, ChangePeerIdWrongCaller) {
    initEmptyState();
    initDefaultMinerInfo();

    callerIs(kInitAddress);

    const Bytes new_peer_id{"0102"_unhex};

    EXPECT_OUTCOME_ERROR(
        asAbort(VMExitCode::kSysErrForbidden),
        ChangePeerId::call(runtime, ChangePeerId::Params{new_peer_id}));
  }

  /**
   * @given vm
   * @when miner ChangePeerId called
   * @then new peer id is recorded to miner info
   */
  TEST_F(MinerActorTest, ChangePeerIdSuccess) {
    initEmptyState();
    initDefaultMinerInfo();

    callerIs(owner);

    const Bytes new_peer_id{"0102"_unhex};

    EXPECT_OUTCOME_TRUE_1(
        ChangePeerId::call(runtime, ChangePeerId::Params{new_peer_id}));

    EXPECT_OUTCOME_TRUE(miner_info, state->getInfo());
    EXPECT_EQ(miner_info->peer_id, new_peer_id);
  }

  TEST_F(MinerActorTest, SubmitWindowedPoStWrongParams) {
    initEmptyState();
    initDefaultMinerInfo();

    callerIs(owner);

    const uint64_t expected_deadline_id = 1;
    const uint64_t wrong_deadline_id = 3;

    state->current_deadline = expected_deadline_id;
    state->proving_period_start =
        current_epoch - 10 - expected_deadline_id * kWPoStChallengeWindow;

    const ChainEpoch chain_commit_epoch = current_epoch - 10;

    const PoStProof post_proof{
        .registered_proof = RegisteredPoStProof::kStackedDRG32GiBWindowPoSt,
        .proof = {}};

    const PoStProof wrong_post_proof{
        .registered_proof = RegisteredPoStProof::kStackedDRG2KiBWindowPoSt,
        .proof = {}};

    EXPECT_OUTCOME_TRUE(
        expected_randomness,
        Randomness::fromString("i_am_random_____i_am_random_____"));

    EXPECT_OUTCOME_TRUE(
        wrong_randomness,
        Randomness::fromString("wrong_random____wrong_random____"));

    EXPECT_CALL(runtime, getRandomnessFromTickets(_, _, _))
        .WillRepeatedly(Return(expected_randomness));

    EXPECT_OUTCOME_ERROR(
        asAbort(VMExitCode::kErrIllegalArgument),
        SubmitWindowedPoSt::call(
            runtime,
            SubmitWindowedPoSt::Params{.deadline = kWPoStPeriodDeadlines,
                                       .partitions = {},
                                       .proofs = {},
                                       .chain_commit_epoch = {},
                                       .chain_commit_rand = {}}));

    EXPECT_OUTCOME_ERROR(
        asAbort(VMExitCode::kErrIllegalArgument),
        SubmitWindowedPoSt::call(
            runtime,
            SubmitWindowedPoSt::Params{.deadline = expected_deadline_id,
                                       .partitions = {},
                                       .proofs = {},
                                       .chain_commit_epoch = {},
                                       .chain_commit_rand = {}}));

    EXPECT_OUTCOME_ERROR(
        asAbort(VMExitCode::kErrIllegalArgument),
        SubmitWindowedPoSt::call(
            runtime,
            SubmitWindowedPoSt::Params{.deadline = expected_deadline_id,
                                       .partitions = {},
                                       .proofs = {2, PoStProof()},
                                       .chain_commit_epoch = {},
                                       .chain_commit_rand = {}}));

    EXPECT_OUTCOME_ERROR(
        asAbort(VMExitCode::kErrIllegalArgument),
        SubmitWindowedPoSt::call(
            runtime,
            SubmitWindowedPoSt::Params{.deadline = expected_deadline_id,
                                       .partitions = {},
                                       .proofs = {wrong_post_proof},
                                       .chain_commit_epoch = {},
                                       .chain_commit_rand = {}}));

    EXPECT_OUTCOME_ERROR(
        asAbort(VMExitCode::kErrIllegalArgument),
        SubmitWindowedPoSt::call(
            runtime,
            SubmitWindowedPoSt::Params{.deadline = expected_deadline_id,
                                       .partitions = {5, PoStPartition()},
                                       .proofs = {post_proof},
                                       .chain_commit_epoch = {},
                                       .chain_commit_rand = {}}));

    EXPECT_OUTCOME_ERROR(
        asAbort(VMExitCode::kErrIllegalArgument),
        SubmitWindowedPoSt::call(
            runtime,
            SubmitWindowedPoSt::Params{.deadline = wrong_deadline_id,
                                       .partitions = {},
                                       .proofs = {post_proof},
                                       .chain_commit_epoch = {},
                                       .chain_commit_rand = {}}));

    EXPECT_OUTCOME_ERROR(
        asAbort(VMExitCode::kErrIllegalArgument),
        SubmitWindowedPoSt::call(runtime,
                                 SubmitWindowedPoSt::Params{
                                     .deadline = expected_deadline_id,
                                     .partitions = {},
                                     .proofs = {post_proof},
                                     .chain_commit_epoch = current_epoch - 1000,
                                     .chain_commit_rand = {}}));

    EXPECT_OUTCOME_ERROR(
        asAbort(VMExitCode::kErrIllegalArgument),
        SubmitWindowedPoSt::call(
            runtime,
            SubmitWindowedPoSt::Params{.deadline = expected_deadline_id,
                                       .partitions = {},
                                       .proofs = {post_proof},
                                       .chain_commit_epoch = current_epoch + 1,
                                       .chain_commit_rand = {}}));

    EXPECT_OUTCOME_ERROR(
        asAbort(VMExitCode::kErrIllegalArgument),
        SubmitWindowedPoSt::call(
            runtime,
            SubmitWindowedPoSt::Params{.deadline = expected_deadline_id,
                                       .partitions = {},
                                       .proofs = {post_proof},
                                       .chain_commit_epoch = chain_commit_epoch,
                                       .chain_commit_rand = wrong_randomness}));
  }

  TEST_F(MinerActorTest, SubmitWindowedPoStSuccess) {
    initEmptyState();
    initDefaultMinerInfo();

    callerIs(owner);
    balance = 1000;

    const uint64_t deadline_id = 1;
    const uint64_t partition_id = 0;

    state->current_deadline = deadline_id;
    state->proving_period_start =
        current_epoch - 10 - deadline_id * kWPoStChallengeWindow;

    const ChainEpoch chain_commit_epoch = current_epoch - 10;

    std::vector<SectorOnChainInfo> sectors;
    for (uint64_t i = 0; i < 4; i++) {
      SectorOnChainInfo sector;
      sector.sector = i;
      sector.sealed_cid = kEmptyObjectCid;
      sectors.push_back(sector);
    }

    EXPECT_OUTCOME_TRUE_1(state->sectors.store(sectors));

    Universal<Partition> partition{actor_version};
    cbor_blake::cbLoadT(ipld, partition);
    partition->sectors = {0, 1, 2, 3};
    partition->faults = {2, 3};

    Universal<Deadline> deadline{actor_version};
    cbor_blake::cbLoadT(ipld, deadline);
    EXPECT_OUTCOME_TRUE_1(deadline->partitions.set(partition_id, partition));

    EXPECT_OUTCOME_TRUE(deadlines, state->deadlines.get());
    EXPECT_OUTCOME_TRUE_1(deadlines.due[deadline_id].set(deadline));
    EXPECT_OUTCOME_TRUE_1(state->deadlines.set(deadlines));

    const PoStProof post_proof{
        .registered_proof = RegisteredPoStProof::kStackedDRG32GiBWindowPoSt,
        .proof = {}};

    EXPECT_OUTCOME_TRUE(
        randomness, Randomness::fromString("i_am_random_____i_am_random_____"));

    EXPECT_CALL(runtime, getRandomnessFromTickets(_, _, _))
        .WillOnce(Return(randomness));

    EXPECT_CALL(runtime, getRandomnessFromBeacon(_, _, _))
        .WillOnce(Return(randomness));

    EXPECT_CALL(runtime, verifyPoSt(_)).WillOnce(Return(true));

    EXPECT_OUTCOME_TRUE_1(SubmitWindowedPoSt::call(
        runtime,
        SubmitWindowedPoSt::Params{
            .deadline = deadline_id,
            .partitions = {{.index = partition_id, .skipped = {}}},
            .proofs = {post_proof},
            .chain_commit_epoch = chain_commit_epoch,
            .chain_commit_rand = randomness}));
  }

  TEST_F(MinerActorTest, PreCommitSectorWrongParams) {
    initEmptyState();
    initDefaultMinerInfo();

    callerIs(owner);

    SectorPreCommitInfo params;

    params.registered_proof = RegisteredSealProof::kStackedDrg2KiBV1;
    EXPECT_OUTCOME_ERROR(asAbort(VMExitCode::kErrIllegalArgument),
                         PreCommitSector::call(runtime, params));

    params.registered_proof = RegisteredSealProof::kStackedDrg64GiBV1;
    params.sector = kMaxSectorNumber + 1;
    EXPECT_OUTCOME_ERROR(asAbort(VMExitCode::kErrIllegalArgument),
                         PreCommitSector::call(runtime, params));

    params.sector = 100;
    EXPECT_OUTCOME_ERROR(asAbort(VMExitCode::kErrIllegalArgument),
                         PreCommitSector::call(runtime, params));

    params.sealed_cid = kEmptyObjectCid;
    EXPECT_OUTCOME_ERROR(asAbort(VMExitCode::kErrIllegalArgument),
                         PreCommitSector::call(runtime, params));

    params.sealed_cid =
        replicaCommitmentV1ToCID(std::vector<uint8_t>(32, 'x')).value();
    params.seal_epoch = current_epoch + 1;
    EXPECT_OUTCOME_ERROR(asAbort(VMExitCode::kErrIllegalArgument),
                         PreCommitSector::call(runtime, params));

    params.seal_epoch = current_epoch - kMaxPreCommitRandomnessLookback - 1;
    EXPECT_OUTCOME_ERROR(asAbort(VMExitCode::kErrIllegalArgument),
                         PreCommitSector::call(runtime, params));

    params.seal_epoch = current_epoch - 10;
    params.expiration = current_epoch - 1;
    EXPECT_OUTCOME_ERROR(asAbort(VMExitCode::kErrIllegalArgument),
                         PreCommitSector::call(runtime, params));

    params.expiration = current_epoch + 10000 + kMinSectorExpiration + 100;
    params.replace_capacity = true;
    EXPECT_OUTCOME_ERROR(asAbort(VMExitCode::kErrIllegalArgument),
                         PreCommitSector::call(runtime, params));

    params.replace_capacity = false;
    params.replace_deadline = kWPoStPeriodDeadlines + 1;
    EXPECT_OUTCOME_ERROR(asAbort(VMExitCode::kErrIllegalArgument),
                         PreCommitSector::call(runtime, params));

    params.replace_deadline = 10;
    params.replace_sector = kMaxSectorNumber + 1;
    EXPECT_OUTCOME_ERROR(asAbort(VMExitCode::kErrIllegalArgument),
                         PreCommitSector::call(runtime, params));

    params.replace_sector = 200;
    expectThisEpochReward({10, 10}, 10);
    expectCurrentTotalPower(100, 100, 1000, {10, 10});
    expectDealWeight(
        params.deal_ids, current_epoch, params.expiration, 1000, 100, 100);
    EXPECT_OUTCOME_ERROR(asAbort(VMExitCode::kErrIllegalArgument),
                         PreCommitSector::call(runtime, params));

    params.registered_proof = RegisteredSealProof::kStackedDrg32GiBV1;
    params.deal_ids = std::vector<DealId>(1000, 0);
    expectThisEpochReward({10, 10}, 10);
    expectCurrentTotalPower(100, 100, 1000, {10, 10});
    expectDealWeight(
        params.deal_ids, current_epoch, params.expiration, 1000, 100, 100);
    EXPECT_OUTCOME_ERROR(asAbort(VMExitCode::kErrIllegalArgument),
                         PreCommitSector::call(runtime, params));
  }

  TEST_F(MinerActorTest, PreCommitSectorSuccess) {
    initEmptyState();
    initDefaultMinerInfo();

    callerIs(owner);
    balance = 1000;

    const SectorNumber sector_num = 100;
    const SectorNumber replace_sector = 200;
    const uint64_t deadline_id = 1;
    const uint64_t partition_id = 0;

    const SectorPreCommitInfo params{
        .registered_proof = RegisteredSealProof::kStackedDrg32GiBV1,
        .sector = sector_num,
        .sealed_cid =
            replicaCommitmentV1ToCID(std::vector<uint8_t>(32, 'x')).value(),
        .seal_epoch = current_epoch - 10,
        .deal_ids = {0, 1, 2, 3},
        .expiration = current_epoch + 2 * kMinSectorExpiration,
        .replace_capacity = true,
        .replace_deadline = deadline_id,
        .replace_partition = partition_id,
        .replace_sector = replace_sector};

    SectorOnChainInfo sector;
    sector.sector = replace_sector;
    sector.sealed_cid = kEmptyObjectCid;
    sector.seal_proof = RegisteredSealProof::kStackedDrg32GiBV1;
    sector.expiration = current_epoch + kMinSectorExpiration;
    sector.init_pledge = 100;
    EXPECT_OUTCOME_TRUE_1(state->sectors.store({sector}));

    Universal<Partition> partition{actor_version};
    cbor_blake::cbLoadT(ipld, partition);
    partition->sectors = {sector_num, replace_sector};

    Universal<Deadline> deadline{actor_version};
    cbor_blake::cbLoadT(ipld, deadline);
    EXPECT_OUTCOME_TRUE_1(deadline->partitions.set(partition_id, partition));

    EXPECT_OUTCOME_TRUE(deadlines, state->deadlines.get());
    EXPECT_OUTCOME_TRUE_1(deadlines.due[deadline_id].set(deadline));
    EXPECT_OUTCOME_TRUE_1(state->deadlines.set(deadlines));

    expectThisEpochReward({10, 10}, 10);
    expectCurrentTotalPower(100, 100, 1000, {10, 10});
    expectDealWeight(
        params.deal_ids, current_epoch, params.expiration, 1000, 100, 100);

    EXPECT_OUTCOME_TRUE_1(PreCommitSector::call(runtime, params));
  }

}  // namespace fc::vm::actor::builtin::v2::miner
