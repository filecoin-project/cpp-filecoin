/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v0/miner/types/expiration.hpp"

#include <gtest/gtest.h>

#include "storage/ipfs/impl/in_memory_datastore.hpp"
#include "test_utils.hpp"
#include "testutil/outcome.hpp"
#include "vm/actor/builtin/types/miner/policy.hpp"

namespace fc::vm::actor::builtin::v0::miner {
  using primitives::ChainEpoch;
  using primitives::DealWeight;
  using primitives::RleBitset;
  using primitives::SectorNumber;
  using primitives::SectorSize;
  using primitives::TokenAmount;
  using storage::ipfs::InMemoryDatastore;
  using types::miner::powerForSectors;
  using types::miner::QuantSpec;

  struct ExpirationQueueTestV0 : testing::Test {
    void SetUp() override {
      eq.quant = types::miner::kNoQuantization;
      ipld->actor_version = ActorVersion::kVersion0;
      cbor_blake::cbLoadT(ipld, eq);

      sectors = {Utils::testSector(2, 1, 50, 60, 1000),
                 Utils::testSector(3, 2, 51, 61, 1001),
                 Utils::testSector(7, 3, 52, 62, 1002),
                 Utils::testSector(8, 4, 53, 63, 1003),
                 Utils::testSector(11, 5, 54, 64, 1004),
                 Utils::testSector(13, 6, 55, 65, 1005)};
    }

    std::shared_ptr<InMemoryDatastore> ipld{
        std::make_shared<InMemoryDatastore>()};

    std::vector<SectorOnChainInfo> sectors;
    SectorSize ssize{static_cast<uint64_t>(32) << 30};

    ExpirationQueue eq;
  };

  TEST_F(ExpirationQueueTestV0, AddedSectorsCanBePoppedOffQueue) {
    EXPECT_OUTCOME_TRUE(result, eq.addActiveSectors(sectors, ssize));
    const auto &[sec_nums, power, pledge] = result;

    const RleBitset expected_sec_nums{1, 2, 3, 4, 5, 6};

    EXPECT_EQ(sec_nums, expected_sec_nums);
    EXPECT_EQ(power, powerForSectors(ssize, sectors));
    EXPECT_EQ(pledge, 6015);
    EXPECT_OUTCOME_EQ(eq.queue.size(), sectors.size());

    EXPECT_OUTCOME_TRUE(es1, eq.popUntil(7));
    EXPECT_OUTCOME_EQ(eq.queue.size(), 3);

    const RleBitset expected_on_time_sectors1{1, 2, 3};
    EXPECT_EQ(es1.on_time_sectors, expected_on_time_sectors1);
    EXPECT_TRUE(es1.early_sectors.empty());
    EXPECT_EQ(es1.on_time_pledge, 3003);
    EXPECT_EQ(es1.active_power,
              powerForSectors(ssize, Utils::slice(sectors, 0, 3)));
    EXPECT_EQ(es1.faulty_power, PowerPair());

    EXPECT_OUTCOME_TRUE(es2, eq.popUntil(20));
    EXPECT_OUTCOME_EQ(eq.queue.size(), 0);

    const RleBitset expected_on_time_sectors2{4, 5, 6};
    EXPECT_EQ(es2.on_time_sectors, expected_on_time_sectors2);
    EXPECT_TRUE(es2.early_sectors.empty());
    EXPECT_EQ(es2.on_time_pledge, 3012);
    EXPECT_EQ(es2.active_power,
              powerForSectors(ssize, Utils::slice(sectors, 3)));
    EXPECT_EQ(es2.faulty_power, PowerPair());
  }

  TEST_F(ExpirationQueueTestV0, QuantizesAddedSectorsByExpiration) {
    eq.quant = QuantSpec(5, 3);

    EXPECT_OUTCOME_TRUE(result, eq.addActiveSectors(sectors, ssize));
    const auto &[sec_nums, power, pledge] = result;

    const RleBitset expected_sec_nums{1, 2, 3, 4, 5, 6};

    EXPECT_EQ(sec_nums, expected_sec_nums);
    EXPECT_EQ(power, powerForSectors(ssize, sectors));
    EXPECT_EQ(pledge, 6015);
    EXPECT_OUTCOME_EQ(eq.queue.size(), 3);

    EXPECT_OUTCOME_TRUE(es1, eq.popUntil(2));
    EXPECT_TRUE(es1.on_time_sectors.empty());
    EXPECT_OUTCOME_EQ(eq.queue.size(), 3);

    EXPECT_OUTCOME_TRUE(es2, eq.popUntil(3));
    const RleBitset expected_on_time_sectors2{1, 2};
    EXPECT_EQ(es2.on_time_sectors, expected_on_time_sectors2);
    EXPECT_OUTCOME_EQ(eq.queue.size(), 2);

    EXPECT_OUTCOME_TRUE(es3, eq.popUntil(7));
    EXPECT_TRUE(es3.on_time_sectors.empty());
    EXPECT_OUTCOME_EQ(eq.queue.size(), 2);

    EXPECT_OUTCOME_TRUE(es4, eq.popUntil(8));
    const RleBitset expected_on_time_sectors4{3, 4};
    EXPECT_EQ(es4.on_time_sectors, expected_on_time_sectors4);
    EXPECT_OUTCOME_EQ(eq.queue.size(), 1);

    EXPECT_OUTCOME_TRUE(es5, eq.popUntil(12));
    EXPECT_TRUE(es5.on_time_sectors.empty());
    EXPECT_OUTCOME_EQ(eq.queue.size(), 1);

    EXPECT_OUTCOME_TRUE(es6, eq.popUntil(13));
    const RleBitset expected_on_time_sectors6{5, 6};
    EXPECT_EQ(es6.on_time_sectors, expected_on_time_sectors6);
    EXPECT_OUTCOME_EQ(eq.queue.size(), 0);
  }

  TEST_F(ExpirationQueueTestV0, ReschedulesSectorsToExpireLater) {
    EXPECT_OUTCOME_TRUE_1(eq.addActiveSectors(sectors, ssize));
    EXPECT_OUTCOME_TRUE_1(
        eq.rescheduleExpirations(20, Utils::slice(sectors, 0, 3), ssize));

    EXPECT_OUTCOME_EQ(eq.queue.size(), 4);

    EXPECT_OUTCOME_TRUE(es1, eq.popUntil(7));
    EXPECT_TRUE(es1.on_time_sectors.empty());
    EXPECT_OUTCOME_EQ(eq.queue.size(), 4);

    EXPECT_OUTCOME_TRUE_1(eq.popUntil(19));
    EXPECT_OUTCOME_EQ(eq.queue.size(), 1);

    EXPECT_OUTCOME_TRUE(es2, eq.popUntil(20));
    EXPECT_OUTCOME_EQ(eq.queue.size(), 0);

    const RleBitset expected_on_time_sectors2{1, 2, 3};
    EXPECT_EQ(es2.on_time_sectors, expected_on_time_sectors2);
    EXPECT_TRUE(es2.early_sectors.empty());

    EXPECT_EQ(es2.on_time_pledge, 3003);
    EXPECT_EQ(es2.active_power,
              powerForSectors(ssize, Utils::slice(sectors, 3)));
    EXPECT_EQ(es2.faulty_power, PowerPair());
  }

  TEST_F(ExpirationQueueTestV0, ReschedulesSectorsAsFaults) {
    eq.quant = QuantSpec(4, 1);
    EXPECT_OUTCOME_TRUE_1(eq.addActiveSectors(sectors, ssize));

    EXPECT_OUTCOME_TRUE(
        power_delta,
        eq.rescheduleAsFaults(6, Utils::slice(sectors, 1, 5), ssize));
    EXPECT_EQ(power_delta, powerForSectors(ssize, Utils::slice(sectors, 1, 5)));

    Utils::requireNoExpirationGroupsBefore(5, eq);
    EXPECT_OUTCOME_TRUE(es1, eq.popUntil(5));
    const RleBitset expected_on_time_sectors1{1, 2};
    EXPECT_EQ(es1.on_time_sectors, expected_on_time_sectors1);
    EXPECT_TRUE(es1.early_sectors.empty());
    EXPECT_EQ(es1.on_time_pledge, 2001);
    EXPECT_EQ(es1.active_power,
              powerForSectors(ssize, Utils::slice(sectors, 0, 1)));
    EXPECT_EQ(es1.faulty_power,
              powerForSectors(ssize, Utils::slice(sectors, 1, 2)));

    Utils::requireNoExpirationGroupsBefore(9, eq);
    EXPECT_OUTCOME_TRUE(es2, eq.popUntil(9));
    const RleBitset expected_on_time_sectors2{3, 4};
    EXPECT_EQ(es2.on_time_sectors, expected_on_time_sectors2);
    const RleBitset expected_early_sectors2{5};
    EXPECT_EQ(es2.early_sectors, expected_early_sectors2);
    EXPECT_EQ(es2.on_time_pledge, 2005);
    EXPECT_EQ(es2.active_power, PowerPair());
    EXPECT_EQ(es2.faulty_power,
              powerForSectors(ssize, Utils::slice(sectors, 2, 5)));

    Utils::requireNoExpirationGroupsBefore(13, eq);
    EXPECT_OUTCOME_TRUE(es3, eq.popUntil(13));
    const RleBitset expected_on_time_sectors3{6};
    EXPECT_EQ(es3.on_time_sectors, expected_on_time_sectors3);
    EXPECT_TRUE(es3.early_sectors.empty());
    EXPECT_EQ(es3.on_time_pledge, 1005);
    EXPECT_EQ(es3.active_power,
              powerForSectors(ssize, Utils::slice(sectors, 5)));
    EXPECT_EQ(es3.faulty_power, PowerPair());
  }

  TEST_F(ExpirationQueueTestV0, ReschedulesAllSectorsAsFaults) {
    eq.quant = QuantSpec(4, 1);
    EXPECT_OUTCOME_TRUE_1(eq.addActiveSectors(sectors, ssize));

    EXPECT_OUTCOME_TRUE_1(eq.rescheduleAllAsFaults(6));

    Utils::requireNoExpirationGroupsBefore(5, eq);
    EXPECT_OUTCOME_TRUE(es1, eq.popUntil(5));
    const RleBitset expected_on_time_sectors1{1, 2};
    EXPECT_EQ(es1.on_time_sectors, expected_on_time_sectors1);
    EXPECT_TRUE(es1.early_sectors.empty());
    EXPECT_EQ(es1.on_time_pledge, 2001);
    EXPECT_EQ(es1.active_power, PowerPair());
    EXPECT_EQ(es1.faulty_power,
              powerForSectors(ssize, Utils::slice(sectors, 0, 2)));

    Utils::requireNoExpirationGroupsBefore(9, eq);
    EXPECT_OUTCOME_TRUE(es2, eq.popUntil(9));
    const RleBitset expected_on_time_sectors2{3, 4};
    EXPECT_EQ(es2.on_time_sectors, expected_on_time_sectors2);
    const RleBitset expected_early_sectors2{5, 6};
    EXPECT_EQ(es2.early_sectors, expected_early_sectors2);
    EXPECT_EQ(es2.on_time_pledge, 2005);
    EXPECT_EQ(es2.active_power, PowerPair());
    EXPECT_EQ(es2.faulty_power,
              powerForSectors(ssize, Utils::slice(sectors, 2)));

    Utils::requireNoExpirationGroupsBefore(13, eq);
    EXPECT_OUTCOME_TRUE(es3, eq.popUntil(13));
    EXPECT_TRUE(es3.on_time_sectors.empty());
    EXPECT_TRUE(es3.early_sectors.empty());
    EXPECT_EQ(es3.on_time_pledge, 0);
    EXPECT_EQ(es3.active_power, PowerPair());
    EXPECT_EQ(es3.faulty_power, PowerPair());
  }

  TEST_F(ExpirationQueueTestV0, RescheduleRecoverRestoresAllSectorStats) {
    eq.quant = QuantSpec(4, 1);
    EXPECT_OUTCOME_TRUE_1(eq.addActiveSectors(sectors, ssize));

    EXPECT_OUTCOME_TRUE_1(
        eq.rescheduleAsFaults(6, Utils::slice(sectors, 1, 5), ssize));

    EXPECT_OUTCOME_TRUE(
        recovered, eq.rescheduleRecovered(Utils::slice(sectors, 1, 5), ssize));
    EXPECT_EQ(recovered, powerForSectors(ssize, Utils::slice(sectors, 1, 5)));

    Utils::requireNoExpirationGroupsBefore(5, eq);
    EXPECT_OUTCOME_TRUE(es1, eq.popUntil(5));
    const RleBitset expected_on_time_sectors1{1, 2};
    EXPECT_EQ(es1.on_time_sectors, expected_on_time_sectors1);
    EXPECT_TRUE(es1.early_sectors.empty());
    EXPECT_EQ(es1.on_time_pledge, 2001);
    EXPECT_EQ(es1.active_power,
              powerForSectors(ssize, Utils::slice(sectors, 0, 2)));
    EXPECT_EQ(es1.faulty_power, PowerPair());

    Utils::requireNoExpirationGroupsBefore(9, eq);
    EXPECT_OUTCOME_TRUE(es2, eq.popUntil(9));
    const RleBitset expected_on_time_sectors2{3, 4};
    EXPECT_EQ(es2.on_time_sectors, expected_on_time_sectors2);
    EXPECT_TRUE(es2.early_sectors.empty());
    EXPECT_EQ(es2.on_time_pledge, 2005);
    EXPECT_EQ(es2.active_power,
              powerForSectors(ssize, Utils::slice(sectors, 2, 4)));
    EXPECT_EQ(es2.faulty_power, PowerPair());

    Utils::requireNoExpirationGroupsBefore(13, eq);
    EXPECT_OUTCOME_TRUE(es3, eq.popUntil(13));
    const RleBitset expected_on_time_sectors3{5, 6};
    EXPECT_EQ(es3.on_time_sectors, expected_on_time_sectors3);
    EXPECT_TRUE(es3.early_sectors.empty());
    EXPECT_EQ(es3.on_time_pledge, 2009);
    EXPECT_EQ(es3.active_power,
              powerForSectors(ssize, Utils::slice(sectors, 4)));
    EXPECT_EQ(es3.faulty_power, PowerPair());
  }

  TEST_F(ExpirationQueueTestV0, ReplacesSectorsWithNewSectors) {
    eq.quant = QuantSpec(4, 1);
    EXPECT_OUTCOME_TRUE_1(eq.addActiveSectors(
        {sectors[0], sectors[1], sectors[3], sectors[5]}, ssize));

    const std::vector<SectorOnChainInfo> to_remove{
        sectors[0], sectors[1], sectors[3]};
    const std::vector<SectorOnChainInfo> to_add{sectors[2], sectors[4]};

    EXPECT_OUTCOME_TRUE(result, eq.replaceSectors(to_remove, to_add, ssize));
    const auto &[removed, added, power_delta, pledge_delta] = result;

    const RleBitset expected_removed{1, 2, 4};
    EXPECT_EQ(removed, expected_removed);
    const RleBitset expected_added{3, 5};
    EXPECT_EQ(added, expected_added);

    const auto added_power = powerForSectors(ssize, to_add);
    EXPECT_EQ(power_delta, added_power - powerForSectors(ssize, to_remove));
    EXPECT_EQ(pledge_delta, 1002 + 1004 - 1000 - 1001 - 1003);

    Utils::requireNoExpirationGroupsBefore(9, eq);
    EXPECT_OUTCOME_TRUE(es1, eq.popUntil(9));
    const RleBitset expected_on_time_sectors1{3};
    EXPECT_EQ(es1.on_time_sectors, expected_on_time_sectors1);
    EXPECT_TRUE(es1.early_sectors.empty());
    EXPECT_EQ(es1.on_time_pledge, 1002);
    EXPECT_EQ(es1.active_power,
              powerForSectors(ssize, Utils::slice(sectors, 2, 3)));
    EXPECT_EQ(es1.faulty_power, PowerPair());

    Utils::requireNoExpirationGroupsBefore(13, eq);
    EXPECT_OUTCOME_TRUE(es2, eq.popUntil(13));
    const RleBitset expected_on_time_sectors2{5, 6};
    EXPECT_EQ(es2.on_time_sectors, expected_on_time_sectors2);
    EXPECT_TRUE(es2.early_sectors.empty());
    EXPECT_EQ(es2.on_time_pledge, 2009);
    EXPECT_EQ(es2.active_power,
              powerForSectors(ssize, Utils::slice(sectors, 4)));
    EXPECT_EQ(es2.faulty_power, PowerPair());
  }

  TEST_F(ExpirationQueueTestV0, RemovesSectors) {
    eq.quant = QuantSpec(4, 1);
    EXPECT_OUTCOME_TRUE_1(eq.addActiveSectors(sectors, ssize));

    EXPECT_OUTCOME_TRUE_1(
        eq.rescheduleAsFaults(6, Utils::slice(sectors, 1, 6), ssize));

    const std::vector<SectorOnChainInfo> to_remove{
        sectors[0], sectors[3], sectors[4], sectors[5]};
    const RleBitset faults{4, 5, 6};
    const RleBitset recovering{6};

    EXPECT_OUTCOME_TRUE(result,
                        eq.removeSectors(to_remove, faults, recovering, ssize));
    const auto &[removed, recovering_power] = result;

    const RleBitset expected_removed_on_time_sectors{1, 4};
    EXPECT_EQ(removed.on_time_sectors, expected_removed_on_time_sectors);
    const RleBitset expected_removed_early_sectors{5, 6};
    EXPECT_EQ(removed.early_sectors, expected_removed_early_sectors);
    EXPECT_EQ(removed.on_time_pledge, 1000 + 1003);
    EXPECT_EQ(removed.active_power, powerForSectors(ssize, {sectors[0]}));
    EXPECT_EQ(removed.faulty_power,
              powerForSectors(ssize, Utils::slice(sectors, 3, 6)));
    EXPECT_EQ(recovering_power,
              powerForSectors(ssize, Utils::slice(sectors, 5, 6)));

    Utils::requireNoExpirationGroupsBefore(5, eq);
    EXPECT_OUTCOME_TRUE(es1, eq.popUntil(5));
    const RleBitset expected_on_time_sectors1{2};
    EXPECT_EQ(es1.on_time_sectors, expected_on_time_sectors1);
    EXPECT_TRUE(es1.early_sectors.empty());
    EXPECT_EQ(es1.on_time_pledge, 1001);
    EXPECT_EQ(es1.active_power, PowerPair());
    EXPECT_EQ(es1.faulty_power,
              powerForSectors(ssize, Utils::slice(sectors, 1, 2)));

    Utils::requireNoExpirationGroupsBefore(9, eq);
    EXPECT_OUTCOME_TRUE(es2, eq.popUntil(9));
    const RleBitset expected_on_time_sectors2{3};
    EXPECT_EQ(es2.on_time_sectors, expected_on_time_sectors2);
    EXPECT_TRUE(es2.early_sectors.empty());
    EXPECT_EQ(es2.on_time_pledge, 1002);
    EXPECT_EQ(es2.active_power, PowerPair());
    EXPECT_EQ(es2.faulty_power,
              powerForSectors(ssize, Utils::slice(sectors, 2, 3)));

    Utils::requireNoExpirationGroupsBefore(20, eq);
  }

  TEST_F(ExpirationQueueTestV0, AddingNoSectorsLeavesTheQueueEmpty) {
    eq.quant = QuantSpec(4, 1);

    EXPECT_OUTCOME_TRUE_1(eq.addActiveSectors({}, ssize));
    EXPECT_OUTCOME_EQ(eq.queue.size(), 0);
  }

  TEST_F(ExpirationQueueTestV0, ReschedulingNoExpirationsLeavesTheQueueEmpty) {
    eq.quant = QuantSpec(4, 1);

    EXPECT_OUTCOME_TRUE_1(eq.rescheduleExpirations(10, {}, ssize));
    EXPECT_OUTCOME_EQ(eq.queue.size(), 0);
  }

  TEST_F(ExpirationQueueTestV0,
         ReschedulingNoExpirationsAsFaultsLeavesTheQueueEmpty) {
    eq.quant = QuantSpec(4, 1);

    EXPECT_OUTCOME_TRUE_1(eq.addActiveSectors(sectors, ssize));
    EXPECT_OUTCOME_TRUE(length, eq.queue.size());
    EXPECT_OUTCOME_TRUE_1(eq.rescheduleAsFaults(15, sectors, ssize));
    EXPECT_OUTCOME_EQ(eq.queue.size(), length);
  }

  TEST_F(ExpirationQueueTestV0,
         ReschedulingAllExpirationsAsFaultsLeavesTheQueueEmptyIfItWasEmpty) {
    eq.quant = QuantSpec(4, 1);

    EXPECT_OUTCOME_TRUE_1(eq.addActiveSectors(sectors, ssize));
    EXPECT_OUTCOME_TRUE(length, eq.queue.size());
    EXPECT_OUTCOME_TRUE_1(eq.rescheduleAllAsFaults(15));
    EXPECT_OUTCOME_EQ(eq.queue.size(), length);
  }

  TEST_F(ExpirationQueueTestV0,
         ReschedulingNoSectorsAsRecoveredLeavesTheQueueEmpty) {
    eq.quant = QuantSpec(4, 1);

    EXPECT_OUTCOME_TRUE_1(eq.rescheduleRecovered({}, ssize));
    EXPECT_OUTCOME_EQ(eq.queue.size(), 0);
  }

}  // namespace fc::vm::actor::builtin::v0::miner
