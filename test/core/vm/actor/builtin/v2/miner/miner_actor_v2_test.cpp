/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v2/miner/miner_actor.hpp"

#include <gtest/gtest.h>
#include "primitives/address/address.hpp"
#include "testutil/literals.hpp"
#include "testutil/outcome.hpp"
#include "testutil/resources/parse.hpp"
#include "testutil/resources/resources.hpp"
#include "testutil/vm/actor/builtin/actor_test_fixture.hpp"
#include "vm/actor/builtin/v2/codes.hpp"
#include "vm/actor/builtin/v2/miner/miner_actor_state.hpp"
#include "vm/actor/builtin/v2/miner/policy.hpp"
#include "vm/actor/builtin/v2/storage_power/storage_power_actor_export.hpp"

namespace fc::vm::actor::builtin::v2::miner {
  using primitives::address::decodeFromString;
  using testing::Return;
  using testutil::vm::actor::builtin::ActorTestFixture;
  using v0::miner::CronEventPayload;
  using v0::miner::CronEventType;
  using v0::miner::Deadline;
  using v0::miner::kWPoStChallengeWindow;
  using v0::miner::kWPoStPeriodDeadlines;

  class MinerActorV2Test : public ActorTestFixture<State> {
   public:
    void SetUp() override {
      ActorTestFixture<State>::SetUp();
      actorVersion = ActorVersion::kVersion2;
      anyCodeIdAddressIs(kAccountCodeId);
    }

    /**
     * Creates simple valid construct parameters
     */
    Construct::Params makeConstructParams() {
      const Address owner = Address::makeFromId(100);
      const Address worker = Address::makeFromId(101);
      return {.owner = owner,
              .worker = worker,
              .control_addresses = {},
              .seal_proof_type = RegisteredSealProof::StackedDrg32GiBV1_1,
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
  TEST_F(MinerActorV2Test, SimpleConstruct) {
    callerIs(kInitAddress);
    currentEpochIs(0);

    const Address worker = Address::makeFromId(101);
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

    EXPECT_OUTCOME_TRUE(miner_info, ipld->getCbor<MinerInfo>(state.info));
    EXPECT_EQ(miner_info.owner, params.owner);
    EXPECT_EQ(miner_info.worker, params.worker);
    EXPECT_EQ(miner_info.control, params.control_addresses);
    EXPECT_EQ(miner_info.peer_id, params.peer_id);
    EXPECT_EQ(miner_info.multiaddrs, params.multiaddresses);
    EXPECT_EQ(static_cast<RegisteredSealProof>(miner_info.seal_proof_type),
              RegisteredSealProof::StackedDrg32GiBV1_1);
    EXPECT_EQ(miner_info.sector_size, BigInt{1} << 35);
    EXPECT_EQ(miner_info.window_post_partition_sectors, 2349);
    EXPECT_EQ(miner_info.consensus_fault_elapsed, kChainEpochUndefined);
    EXPECT_EQ(miner_info.pending_owner_address, boost::none);

    EXPECT_EQ(state.precommit_deposit, 0);
    EXPECT_EQ(state.locked_funds, 0);
    EXPECT_EQ(state.proving_period_start, proving_period_start);
    EXPECT_EQ(state.current_deadline, deadline_index);

    EXPECT_OUTCOME_TRUE(deadlines, ipld->getCbor<Deadlines>(state.deadlines));
    EXPECT_EQ(deadlines.due.size(), kWPoStPeriodDeadlines);

    for (const auto &deadline_cid : deadlines.due) {
      EXPECT_OUTCOME_TRUE(deadline, ipld->getCbor<Deadline>(deadline_cid));
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
  TEST_F(MinerActorV2Test, ConstructResolvedControl) {
    callerIs(kInitAddress);
    currentEpochIs(0);

    const Address worker = Address::makeFromId(101);
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

    EXPECT_OUTCOME_TRUE(miner_info, ipld->getCbor<MinerInfo>(state.info));
    EXPECT_EQ(miner_info.control.size(), 2);
    EXPECT_EQ(miner_info.control[0], controlId1);
    EXPECT_EQ(miner_info.control[1], controlId2);
  }

  /**
   * @given vm and control addresses are not id addresses
   * @when miner constructor called
   * @then error returned
   */
  TEST_F(MinerActorV2Test, ConstructControlNotId) {
    callerIs(kInitAddress);
    currentEpochIs(0);

    const Address worker = Address::makeFromId(101);
    expectAccountV2PubkeyAddressSend(worker, bls_pubkey);

    std::vector<Address> control_addresses;
    const Address control = Address::makeFromId(501);
    control_addresses.emplace_back(control);
    addressCodeIdIs(control, kCronCodeId);

    EXPECT_CALL(runtime, getNetworkVersion())
        .WillRepeatedly(Return(NetworkVersion::kVersion8));

    Construct::Params params = makeConstructParams();
    params.worker = worker;
    params.control_addresses = control_addresses;
    EXPECT_OUTCOME_ERROR(VMAbortExitCode{VMExitCode::kErrIllegalArgument},
                         Construct::call(runtime, params));
  }

  /**
   * @given PeerId too large
   * @when miner constructor called
   * @then error returned
   */
  TEST_F(MinerActorV2Test, ConstructTooLargePeerId) {
    callerIs(kInitAddress);
    const Buffer wrong_peer_id(kMaxPeerIDLength + 1, 'x');

    Construct::Params params = makeConstructParams();
    params.peer_id = wrong_peer_id;
    EXPECT_OUTCOME_ERROR(VMAbortExitCode{VMExitCode::kErrIllegalArgument},
                         Construct::call(runtime, params));
  }

  /**
   * @given control addresses exceed limit
   * @when miner constructor called
   * @then error returned
   */
  TEST_F(MinerActorV2Test, ConstructControlAddressesExceedLimit) {
    callerIs(kInitAddress);
    const Address control = Address::makeFromId(501);
    std::vector<Address> control_addresses(kMaxControlAddresses + 1, control);

    Construct::Params params = makeConstructParams();
    params.control_addresses = control_addresses;
    EXPECT_OUTCOME_ERROR(VMAbortExitCode{VMExitCode::kErrIllegalArgument},
                         Construct::call(runtime, params));
  }

  /**
   * @given multiaddresses size too large
   * @when miner constructor called
   * @then error returned
   */
  TEST_F(MinerActorV2Test, ConstructMultiaddressesTooLarge) {
    callerIs(kInitAddress);
    Multiaddress multiaddress =
        Multiaddress::create("/ip4/127.0.0.1/tcp/111").value();

    Construct::Params params = makeConstructParams();
    params.multiaddresses = std::vector<Multiaddress>(1000, multiaddress);
    EXPECT_OUTCOME_ERROR(VMAbortExitCode{VMExitCode::kErrIllegalArgument},
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
  struct ConstructSuccesssMinerActorV2Test
      : public MinerActorV2Test,
        public ::testing::WithParamInterface<ConstructParams> {};

  /**
   * @given Successful construction with given network version and proof type
   * @when miner constructor called
   * @then success
   */
  TEST_P(ConstructSuccesssMinerActorV2Test, ConstructParametrizedNVSuccess) {
    callerIs(kInitAddress);
    currentEpochIs(0);

    const auto &[netwrok_version, seal_proof_type] = GetParam();

    EXPECT_CALL(runtime, getCurrentReceiver())
        .WillRepeatedly(Return(Address::makeFromId(1000)));
    EXPECT_CALL(runtime, getNetworkVersion())
        .WillRepeatedly(Return(netwrok_version));

    const Address worker = Address::makeFromId(101);
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
      ConstructSuccesssMinerActorV2TestCases,
      ConstructSuccesssMinerActorV2Test,
      ::testing::Values(
          // version < 7 accepts only StackedDrg32GiBV1
          ConstructParams{NetworkVersion::kVersion6,
                          RegisteredSealProof::StackedDrg32GiBV1},
          // version 7 accepts both StackedDrg32GiBV1 and StackedDrg32GiBV1_1
          ConstructParams{NetworkVersion::kVersion7,
                          RegisteredSealProof::StackedDrg32GiBV1},
          ConstructParams{NetworkVersion::kVersion7,
                          RegisteredSealProof::StackedDrg32GiBV1_1},
          // version > 7 accepts only StackedDrg32GiBV1_1
          ConstructParams{NetworkVersion::kVersion8,
                          RegisteredSealProof::StackedDrg32GiBV1_1}));

  /**
   * Failed construction tests
   */
  struct ConstructFailureMinerActorV2Test
      : public MinerActorV2Test,
        public ::testing::WithParamInterface<ConstructParams> {};

  /**
   * @given Construction with wrong network version and proof type
   * @when miner constructor called
   * @then error returned
   */
  TEST_P(ConstructFailureMinerActorV2Test, ConstructParametrizedNVFailure) {
    callerIs(kInitAddress);
    currentEpochIs(0);

    const auto &[netwrok_version, seal_proof_type] = GetParam();

    EXPECT_CALL(runtime, getCurrentReceiver())
        .WillRepeatedly(Return(Address::makeFromId(1000)));
    EXPECT_CALL(runtime, getNetworkVersion())
        .WillRepeatedly(Return(netwrok_version));

    Construct::Params params = makeConstructParams();
    params.seal_proof_type = seal_proof_type;
    EXPECT_OUTCOME_ERROR(VMAbortExitCode{VMExitCode::kErrIllegalArgument},
                         Construct::call(runtime, params));
  }

  INSTANTIATE_TEST_CASE_P(
      ConstructFailureMinerActorV2TestCases,
      ConstructFailureMinerActorV2Test,
      ::testing::Values(
          // version < 7 accepts only StackedDrg32GiBV1
          ConstructParams{NetworkVersion::kVersion6,
                          RegisteredSealProof::StackedDrg32GiBV1_1},
          // version > 7 accepts only StackedDrg32GiBV1_1
          ConstructParams{NetworkVersion::kVersion8,
                          RegisteredSealProof::StackedDrg32GiBV1}));

}  // namespace fc::vm::actor::builtin::v2::miner
