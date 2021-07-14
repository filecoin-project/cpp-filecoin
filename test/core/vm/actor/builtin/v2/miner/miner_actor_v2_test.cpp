/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v2/miner/miner_actor.hpp"

#include "primitives/address/address.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"
#include "testutil/resources/parse.hpp"
#include "testutil/resources/resources.hpp"
#include "testutil/vm/actor/builtin/miner/miner_actor_test_fixture.hpp"
#include "vm/actor/builtin/types/miner/policy.hpp"
#include "vm/actor/builtin/v2/miner/miner_actor_state.hpp"
#include "vm/actor/builtin/v2/miner/types/types.hpp"
#include "vm/actor/builtin/v2/storage_power/storage_power_actor_export.hpp"
#include "vm/actor/codes.hpp"

namespace fc::vm::actor::builtin::v2::miner {
  using primitives::kChainEpochUndefined;
  using primitives::address::decodeFromString;
  using primitives::sector::RegisteredSealProof;
  using testing::Return;
  using testutil::vm::actor::builtin::miner::MinerActorTestFixture;
  using namespace types::miner;

  class MinerActorTest : public MinerActorTestFixture<MinerActorState> {
   public:
    void SetUp() override {
      MinerActorTestFixture<MinerActorState>::SetUp();
      actor_version = ActorVersion::kVersion2;
      ipld->actor_version = actor_version;
      anyCodeIdAddressIs(kAccountCodeId);
      cbor_blake::cbLoadT(ipld, state);
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
          (-proving_period_start) / kWPoStChallengeWindow;
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
    currentEpochIs(0);

    expectAccountV2PubkeyAddressSend(worker, bls_pubkey);

    EXPECT_CALL(runtime, getCurrentReceiver())
        .WillRepeatedly(Return(Address::makeFromId(1000)));
    EXPECT_CALL(runtime, getNetworkVersion())
        .WillRepeatedly(Return(NetworkVersion::kVersion8));

    // This is just set from running the code.
    const ChainEpoch proving_period_start{-2222};
    const auto deadline_index = (-proving_period_start) / kWPoStChallengeWindow;
    expectEnrollCronEvent(proving_period_start);

    Construct::Params params = makeConstructParams();
    params.worker = worker;
    EXPECT_OUTCOME_TRUE_1(Construct::call(runtime, params));

    EXPECT_OUTCOME_TRUE(miner_info, state.getInfo());
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

    EXPECT_EQ(state.precommit_deposit, 0);
    EXPECT_EQ(state.locked_funds, 0);
    EXPECT_EQ(state.proving_period_start, proving_period_start);
    EXPECT_EQ(state.current_deadline, deadline_index);

    EXPECT_OUTCOME_TRUE(deadlines, state.deadlines.get());
    EXPECT_EQ(deadlines.due.size(), kWPoStPeriodDeadlines);

    for (const auto &deadline_cid : deadlines.due) {
      EXPECT_OUTCOME_TRUE(deadline, getCbor<Deadline>(ipld, deadline_cid));
      EXPECT_OUTCOME_EQ(deadline.partitions.size(), 0);
      EXPECT_OUTCOME_EQ(deadline.expirations_epochs.size(), 0);
      EXPECT_TRUE(deadline.post_submissions.empty());
      EXPECT_TRUE(deadline.early_terminations.empty());
      EXPECT_EQ(deadline.live_sectors, 0);
      EXPECT_EQ(deadline.total_sectors, 0);
      EXPECT_EQ(deadline.faulty_power.raw, 0);
      EXPECT_EQ(deadline.faulty_power.qa, 0);
    }
  }

  /**
   * @given vm and control addresses are resolvable
   * @when miner is constructed
   * @then control addresses are resolved
   */
  TEST_F(MinerActorTest, ConstructResolvedControl) {
    callerIs(kInitAddress);
    currentEpochIs(0);

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
    EXPECT_CALL(runtime, getNetworkVersion())
        .WillRepeatedly(Return(NetworkVersion::kVersion8));

    // This is just set from running the code.
    const ChainEpoch proving_period_start{-2222};
    expectEnrollCronEvent(proving_period_start);

    Construct::Params params = makeConstructParams();
    params.worker = worker;
    params.control_addresses = control_addresses;
    EXPECT_OUTCOME_TRUE_1(Construct::call(runtime, params));

    EXPECT_OUTCOME_TRUE(miner_info, state.getInfo());
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
    currentEpochIs(0);

    expectAccountV2PubkeyAddressSend(worker, bls_pubkey);

    std::vector<Address> control_addresses;
    control_addresses.emplace_back(control);
    addressCodeIdIs(control, kCronCodeId);

    EXPECT_CALL(runtime, getNetworkVersion())
        .WillRepeatedly(Return(NetworkVersion::kVersion8));

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
    const Buffer wrong_peer_id(kMaxPeerIDLength + 1, 'x');

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
    currentEpochIs(0);

    const auto &[netwrok_version, seal_proof_type] = GetParam();

    EXPECT_CALL(runtime, getCurrentReceiver())
        .WillRepeatedly(Return(Address::makeFromId(1000)));
    EXPECT_CALL(runtime, getNetworkVersion())
        .WillRepeatedly(Return(netwrok_version));

    expectAccountV2PubkeyAddressSend(worker, bls_pubkey);

    // This is just set from running the code.
    const ChainEpoch proving_period_start{-2222};
    expectEnrollCronEvent(proving_period_start);

    Construct::Params params = makeConstructParams();
    params.worker = worker;
    params.seal_proof_type = seal_proof_type;
    EXPECT_OUTCOME_TRUE_1(Construct::call(runtime, params));
  }

  INSTANTIATE_TEST_CASE_P(
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

  INSTANTIATE_TEST_CASE_P(
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
    currentEpochIs(0);

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

    currentEpochIs(10);
    const ChainEpoch effective_epoch{10 + kWorkerKeyChangeDelay};

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

    EXPECT_OUTCOME_TRUE(miner_info, state.getInfo());
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

    const Buffer new_peer_id{"0102"_unhex};

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

    const Buffer new_peer_id{"0102"_unhex};

    EXPECT_OUTCOME_TRUE_1(
        ChangePeerId::call(runtime, ChangePeerId::Params{new_peer_id}));

    EXPECT_OUTCOME_TRUE(miner_info, state.getInfo());
    EXPECT_EQ(miner_info->peer_id, new_peer_id);
  }

}  // namespace fc::vm::actor::builtin::v2::miner
