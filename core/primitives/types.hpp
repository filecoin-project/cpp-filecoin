/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_PRIMITIVES_TYPES_HPP
#define CPP_FILECOIN_CORE_PRIMITIVES_TYPES_HPP

#include <cstdint>

#include "codec/cbor/streams_annotation.hpp"
#include "primitives/big_int.hpp"
#include "primitives/chain_epoch/chain_epoch.hpp"
#include "primitives/seal_tasks/task.hpp"

namespace fc::primitives {
  using ActorId = uint64_t;

  using TokenAmount = BigInt;

  using TipsetWeight = BigInt;

  using SectorSize = uint64_t;

  using SectorNumber = uint64_t;

  using DealWeight = BigInt;

  using DealId = uint64_t;

  using GasAmount = int64_t;

  // StorageID identifies sector storage by UUID. One sector storage should map
  // to one
  //  filesystem, local or networked / shared by multiple machines
  using StorageID = std::string;

  struct FsStat {
    uint64_t capacity;
    uint64_t available;  // Available to use for sector storage
    uint64_t used;
  };

  inline bool operator==(const FsStat &lhs, const FsStat &rhs) {
    return lhs.capacity == rhs.capacity && lhs.available == rhs.available
           && lhs.used == rhs.used;
  };

  struct StoragePath {
    StorageID id;
    uint64_t weight;

    std::string local_path;

    bool can_seal;
    bool can_store;
  };

  inline bool operator==(const StoragePath &lhs, const StoragePath &rhs) {
    return lhs.id == rhs.id && lhs.weight == rhs.weight
           && lhs.local_path == rhs.local_path && lhs.can_seal == rhs.can_seal
           && lhs.can_store == rhs.can_store;
  };

  struct LocalStorageMeta {
    StorageID id;
    uint64_t weight;  // 0 = readonly

    bool can_seal;
    bool can_store;
  };

  struct SectorStorageWeightDesc {
    SectorSize sector_size;
    EpochDuration duration;
    DealWeight deal_weight;
    DealWeight verified_deal_weight;
  };

  struct WorkerResources {
    uint64_t physical_memory;
    uint64_t swap_memory;

    uint64_t reserved_memory;  // Used by system / other processes

    uint64_t cpus;  // Logical cores
    std::vector<std::string> gpus;
  };

  struct WorkerInfo {
    std::string hostname;
    WorkerResources resources;
  };

  struct WorkerStats {
    WorkerInfo info;

    uint64_t min_used_memory;
    uint64_t max_used_memory;

    bool is_gpu_used;
    uint64_t cpu_use;
  };

  struct PieceDescriptor {
    SectorNumber id;
    uint64_t offset;
    uint64_t length;
  };

  using StoragePower = BigInt;

  using SpaceTime = BigInt;

  using SectorQuality = BigInt;

  CBOR_TUPLE(SectorStorageWeightDesc, sector_size, duration, deal_weight)
}  // namespace fc::primitives

#endif  // CPP_FILECOIN_CORE_PRIMITIVES_TYPES_HPP
