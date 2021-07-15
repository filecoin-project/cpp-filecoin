/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "vm/actor/builtin/v0/miner/types/partition.hpp"

#include <gtest/gtest.h>
#include "storage/ipfs/impl/in_memory_datastore.hpp"
#include "test_utils.hpp"
#include "testutil/mocks/vm/runtime/runtime_mock.hpp"
#include "testutil/outcome.hpp"
#include "vm/actor/builtin/types/miner/policy.hpp"
#include "vm/actor/builtin/types/type_manager/type_manager.hpp"
#include "vm/actor/version.hpp"

namespace fc::vm::actor::builtin::v0::miner {
  using runtime::MockRuntime;
  using storage::ipfs::InMemoryDatastore;
  using testing::_;
  using testing::Return;
  using types::TypeManager;

  struct ExpectExpirationGroup {
    ChainEpoch expiration{};
    RleBitset sectors;
  };

  struct PartitionTestV0 : testing::Test {
    void SetUp() override {
      actor_version = ActorVersion::kVersion0;
      ipld->actor_version = actor_version;

      sectors = {Utils::testSector(2, 1, 50, 60, 1000),
                 Utils::testSector(3, 2, 51, 61, 1001),
                 Utils::testSector(7, 3, 52, 62, 1002),
                 Utils::testSector(8, 4, 53, 63, 1003),
                 Utils::testSector(11, 5, 54, 64, 1004),
                 Utils::testSector(13, 6, 55, 65, 1005)};

      EXPECT_CALL(runtime, getIpfsDatastore())
          .WillRepeatedly(testing::Invoke([&]() { return ipld; }));

      EXPECT_OUTCOME_TRUE(
          power, partition.addSectors(runtime, false, sectors, ssize, quant));

      EXPECT_EQ(power, types::miner::powerForSectors(ssize, sectors));
    }

    void assertPartitionExpirationQueue() const {
      EXPECT_OUTCOME_TRUE(queue,
                          TypeManager::loadExpirationQueue(
                              runtime, partition.expirations_epochs, quant));

      for (const auto &group : groups) {
        Utils::requireNoExpirationGroupsBefore(group.expiration, *queue);
        EXPECT_OUTCOME_TRUE(es, queue->popUntil(group.expiration));

        const auto all_sectors = es.on_time_sectors + es.early_sectors;
        EXPECT_EQ(group.sectors, all_sectors);
      }
    }

    void checkPartitionInvariants() const {
      // todo
    }

    void assertPartitionState(const RleBitset &all_sectors_ids,
                              const RleBitset &faults,
                              const RleBitset &recovering,
                              const RleBitset &terminations) const {
      EXPECT_EQ(partition.faults, faults);
      EXPECT_EQ(partition.recoveries, recovering);
      EXPECT_EQ(partition.terminated, terminations);
      EXPECT_EQ(partition.sectors, all_sectors_ids);

      checkPartitionInvariants();
    }

    std::vector<SectorOnChainInfo> selectSectors(const RleBitset &field) const {
      auto to_include = field;
      std::vector<SectorOnChainInfo> included;

      for (const auto &sector : sectors) {
        if (!to_include.has(sector.sector)) {
          continue;
        }
        included.push_back(sector);
        to_include.erase(sector.sector);
      }
      EXPECT_TRUE(to_include.empty());
      return included;
    }

    std::vector<SectorOnChainInfo> rescheduleSectors(
        ChainEpoch target, const RleBitset &filter) const {
      auto to_reschedule = filter;
      std::vector<SectorOnChainInfo> output;

      for (const auto &sector : sectors) {
        auto sector_copy = sector;
        if (to_reschedule.has(sector_copy.sector)) {
          sector_copy.expiration = target;
        }
        output.push_back(sector_copy);
      }
      return output;
    }

    MockRuntime runtime;
    std::shared_ptr<InMemoryDatastore> ipld{
        std::make_shared<InMemoryDatastore>()};
    ActorVersion actor_version;

    std::vector<SectorOnChainInfo> sectors;
    SectorSize ssize{static_cast<uint64_t>(32) << 30};
    QuantSpec quant{4, 1};

    Partition partition;
    std::vector<ExpectExpirationGroup> groups;
  };

}  // namespace fc::vm::actor::builtin::v0::miner
