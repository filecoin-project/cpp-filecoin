/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/states/storage_power/v2/storage_power_actor_state.hpp"
#include "vm/actor/builtin/v2/storage_power/storage_power_actor_export.hpp"

#include <gtest/gtest.h>
#include <libp2p/peer/peer_id.hpp>
#include "const.hpp"
#include "storage/ipfs/impl/in_memory_datastore.hpp"
#include "testutil/mocks/vm/runtime/runtime_mock.hpp"
#include "vm/actor/builtin/v2/init/init_actor.hpp"
#include "vm/actor/builtin/v2/miner/miner_actor.hpp"
#include "vm/actor/builtin/v2/reward/reward_actor.hpp"
#include "vm/actor/codes.hpp"

namespace fc::vm::actor::builtin::v2::storage_power {
  using libp2p::multi::Multihash;
  using libp2p::peer::PeerId;
  using primitives::ChainEpoch;
  using primitives::kChainEpochUndefined;
  using primitives::SectorNumber;
  using primitives::TokenAmount;
  using primitives::sector::RegisteredSealProof;
  using primitives::sector::SealVerifyInfo;
  using runtime::MockRuntime;

  using storage::ipfs::InMemoryDatastore;
  using testing::Return;
  using types::storage_power::kConsensusMinerMinPower;
  using types::storage_power::kGasOnSubmitVerifySeal;

  class StoragePowerActorV2Test : public testing::Test {
    void SetUp() override {
      actor_version = ActorVersion::kVersion2;
      ipld->actor_version = actor_version;
      cbor_blake::cbLoadT(ipld, state);

      EXPECT_CALL(runtime, getActorVersion())
          .WillRepeatedly(testing::Invoke([&]() { return actor_version; }));

      EXPECT_CALL(runtime, getCurrentEpoch())
          .WillRepeatedly(testing::Invoke([&]() { return current_epoch; }));

      EXPECT_CALL(runtime, getIpfsDatastore()).WillRepeatedly(Return(ipld));

      EXPECT_CALL(runtime, getImmediateCaller())
          .WillRepeatedly(testing::Invoke([&]() { return caller; }));

      EXPECT_CALL(runtime, commit(testing::_))
          .WillRepeatedly(testing::Invoke([&](auto &cid) {
            EXPECT_OUTCOME_TRUE(new_state, getCbor<PowerActorState>(ipld, cid));
            state = std::move(new_state);
            return outcome::success();
          }));

      EXPECT_CALL(runtime, getActorStateCid())
          .WillRepeatedly(testing::Invoke([&]() {
            EXPECT_OUTCOME_TRUE(cid, setCbor(ipld, state));
            return std::move(cid);
          }));
    }

   public:
    /**
     * Storage power state is initialized
     */
    void constructed() {
      caller = kSystemActorAddress;
      EXPECT_OUTCOME_TRUE_1(Construct::call(runtime, {}));
    }

    /**
     * Ensures that caller has code id
     * @param code_id - expected code id
     */
    void callerCodeIdIs(const CodeId &code_id) {
      EXPECT_CALL(runtime, getActorCodeID(caller))
          .WillOnce(Return(code_id));
    }

    /**
     * Creates params for method CreateMiner
     * @return valid CreateMiner parameters
     */
    CreateMiner::Result createMiner(const Address &owner,
                                    const Address &worker,
                                    const Address &id_address,
                                    const Address &robust_address) {
      Buffer peer_id = Buffer(10, 'x');
      CreateMiner::Params params{
          .owner = owner,
          .worker = worker,
          .seal_proof_type = RegisteredSealProof::kStackedDrg32GiBV1,
          .peer_id = peer_id,
          .multiaddresses = {}};
      EXPECT_OUTCOME_TRUE(miner_params,
                          encodeActorParams(miner::Construct::Params{
                              .owner = params.owner,
                              .worker = params.worker,
                              .control_addresses = {},
                              .seal_proof_type = params.seal_proof_type,
                              .peer_id = params.peer_id,
                              .multiaddresses = params.multiaddresses}));
      runtime.expectSendM<init::Exec>(
          kInitAddress,
          {CodeId{kStorageMinerCodeId}, miner_params},
          0,
          {id_address, robust_address});
      EXPECT_CALL(runtime, getValueReceived()).WillOnce(Return(TokenAmount{0}));

      callerCodeIdIs(CodeId{kAccountCodeId});
      EXPECT_OUTCOME_TRUE(result, CreateMiner::call(runtime, params));
      return result;
    }

    void updateClaimedPower(const Address &miner, const TokenAmount &pledge) {
      caller = miner;
      callerCodeIdIs(CodeId{kStorageMinerCodeId});
      EXPECT_OUTCOME_TRUE_1(UpdatePledgeTotal::call(runtime, {pledge}));
    }

    void updateClaimedPower(const Address &miner,
                            const StoragePower &raw_power,
                            const StoragePower &qa_power) {
      caller = miner;
      callerCodeIdIs(CodeId{kStorageMinerCodeId});
      EXPECT_OUTCOME_TRUE_1(
          UpdateClaimedPower::call(runtime, {raw_power, qa_power}));
    }

    MockRuntime runtime;
    ChainEpoch current_epoch{1};
    std::shared_ptr<InMemoryDatastore> ipld{
        std::make_shared<InMemoryDatastore>()};
    Address caller;
    PowerActorState state;
    ActorVersion actor_version;
  };

  /**
   * @given caller is system actor
   * @when Costruction is called
   * @then state is saved and default values are correct
   */
  TEST_F(StoragePowerActorV2Test, SimpleConstruction) {
    caller = kSystemActorAddress;

    EXPECT_OUTCOME_TRUE_1(Construct::call(runtime, {}));

    // values from lotus specs-actors
    EXPECT_EQ(state.total_raw_power, StoragePower{0});
    EXPECT_EQ(state.total_raw_commited, StoragePower{0});
    EXPECT_EQ(state.total_qa_power, StoragePower{0});
    EXPECT_EQ(state.total_qa_commited, StoragePower{0});
    EXPECT_EQ(state.total_pledge_collateral, TokenAmount{0});
    EXPECT_EQ(state.this_epoch_raw_power, StoragePower{0});
    EXPECT_EQ(state.this_epoch_qa_power, StoragePower{0});
    EXPECT_EQ(state.this_epoch_pledge_collateral, TokenAmount{0});
    EXPECT_EQ(state.this_epoch_qa_power_smoothed.position,
              BigInt("274031556999544297163190906134303066185487351808000000"));
    EXPECT_EQ(state.this_epoch_qa_power_smoothed.velocity,
              BigInt("1403041571837666801475537439407631698869695241256960"));
    EXPECT_EQ(state.miner_count, 0);
    EXPECT_EQ(state.num_miners_meeting_min_power, 0);
    EXPECT_EQ(state.first_cron_epoch, ChainEpoch{0});
    EXPECT_OUTCOME_EQ(state.claims.size(), 0);
    EXPECT_OUTCOME_EQ(state.cron_event_queue.size(), 0);
  }

  /**
   * @given storage power actor initialized and empty
   * @when create miner called
   * @then new miner is created
   */
  TEST_F(StoragePowerActorV2Test, CreateMiner) {
    constructed();

    const Address owner = Address::makeFromId(101);
    const Address worker = Address::makeFromId(103);
    const Address id_address = Address::makeFromId(1001);
    const Address robust_address = Address::makeFromId(1003);

    const auto res = createMiner(owner, worker, id_address, robust_address);

    EXPECT_EQ(state.miner_count, 1);
    EXPECT_OUTCOME_TRUE(claim, state.claims.get(id_address));
    EXPECT_EQ(claim->raw_power, StoragePower{0});
    EXPECT_EQ(claim->qa_power, StoragePower{0});
    EXPECT_EQ(res.id_address, id_address);
    EXPECT_EQ(res.robust_address, robust_address);
  }

  /**
   * @given storage power actor initialized and empty and caller is not signable
   * @when create miner called
   * @then error kSysErrForbidden returned
   */
  TEST_F(StoragePowerActorV2Test, CreateMinerCallerNotSignable) {
    constructed();
    callerCodeIdIs(kEmptyObjectCid);

    EXPECT_OUTCOME_ERROR(asAbort(VMExitCode::kSysErrForbidden),
                         CreateMiner::call(runtime, {}));
  }

  /**
   * @given storage power actor initialized and empty and caller is not miner
   * @when UpdateClaimedPower called
   * @then error kSysErrForbidden returned
   */
  TEST_F(StoragePowerActorV2Test, UpdateClaimedPowerCallerNotMiner) {
    constructed();
    callerCodeIdIs(kEmptyObjectCid);

    const UpdateClaimedPower::Params params{};
    EXPECT_OUTCOME_ERROR(asAbort(VMExitCode::kSysErrForbidden),
                         UpdateClaimedPower::call(runtime, params));
  }

  /**
   * @given storage power actor initialized and empty and caller is miner
   * @when UpdateClaimedPower called
   * @then error kErrNotFound returned
   */
  TEST_F(StoragePowerActorV2Test, UpdateClaimedPowerMinerNotFound) {
    constructed();
    callerCodeIdIs(CodeId{kStorageMinerCodeId});

    UpdateClaimedPower::Params params{};
    EXPECT_OUTCOME_ERROR(asAbort(VMExitCode::kErrNotFound),
                         UpdateClaimedPower::call(runtime, params));
  }

  /**
   * @given storage power actor and miner created and one proof submitted
   * @when OnEpochTickEnd called by cron address
   * @then proof verified
   */
  TEST_F(StoragePowerActorV2Test, OneMinerOneSectorPoRepForBulkVerify) {
    constructed();
    const Address owner = Address::makeFromId(101);
    const Address worker = Address::makeFromId(103);
    const Address miner_address = Address::makeFromId(1001);
    const StoragePower power{kConsensusMinerMinPower};
    createMiner(owner, worker, miner_address, miner_address);

    caller = miner_address;
    callerCodeIdIs(CodeId{kStorageMinerCodeId});
    const SectorNumber verified_sector_number = 25;
    SealVerifyInfo seal;
    seal.sector.sector = verified_sector_number;
    seal.sealed_cid = kEmptyObjectCid;
    seal.unsealed_cid = kEmptyObjectCid;
    EXPECT_CALL(runtime, chargeGas(kGasOnSubmitVerifySeal))
        .WillOnce(Return(outcome::success()));
    EXPECT_OUTCOME_TRUE_1(SubmitPoRepForBulkVerify::call(runtime, seal));

    runtime::BatchSealsOut verified_result{
        {miner_address, {verified_sector_number}}};
    EXPECT_CALL(runtime, batchVerifySeals(testing::_))
        .WillOnce(Return(outcome::success(verified_result)));
    runtime.expectSendM<miner::ConfirmSectorProofsValid>(
        miner_address,
        {std::vector<SectorNumber>{verified_sector_number}},
        0,
        {});
    runtime.expectSendM<reward::UpdateNetworkKPI>(
        kRewardAddress, {state.this_epoch_raw_power}, 0, {});

    caller = kCronAddress;
    EXPECT_OUTCOME_TRUE_1(OnEpochTickEnd::call(runtime, {}));
  }
}  // namespace fc::vm::actor::builtin::v2::storage_power
