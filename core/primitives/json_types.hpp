/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "codec/json/coding.hpp"
#include "primitives/types.hpp"

namespace fc::primitives {
  using codec::json::Get;
  using codec::json::Set;
  using codec::json::Value;

  JSON_ENCODE(FsStat) {
    Value j{rapidjson::kObjectType};
    Set(j, "Capacity", v.capacity, allocator);
    Set(j, "Available", v.available, allocator);
    Set(j, "FSAvailable", v.fs_available, allocator);
    Set(j, "Reserved", v.reserved, allocator);
    return j;
  }

  JSON_DECODE(FsStat) {
    Get(j, "Capacity", v.capacity);
    Get(j, "Available", v.available);
    Get(j, "FSAvailable", v.fs_available);
    Get(j, "Reserved", v.reserved);
  }

  JSON_ENCODE(StoragePath) {
    Value j{rapidjson::kObjectType};
    Set(j, "ID", v.id, allocator);
    Set(j, "Weight", v.weight, allocator);
    Set(j, "LocalPath", v.local_path, allocator);
    Set(j, "CanSeal", v.can_seal, allocator);
    Set(j, "CanStore", v.can_store, allocator);
    return j;
  }

  JSON_DECODE(StoragePath) {
    Get(j, "ID", v.id);
    Get(j, "Weight", v.weight);
    Get(j, "LocalPath", v.local_path);
    Get(j, "CanSeal", v.can_seal);
    Get(j, "CanStore", v.can_store);
  }

  JSON_ENCODE(LocalStorageMeta) {
    Value j{rapidjson::kObjectType};
    Set(j, "ID", v.id, allocator);
    Set(j, "Weight", v.weight, allocator);
    Set(j, "CanSeal", v.can_seal, allocator);
    Set(j, "CanStore", v.can_store, allocator);
    return j;
  }

  JSON_DECODE(LocalStorageMeta) {
    Get(j, "ID", v.id);
    Get(j, "Weight", v.weight);
    Get(j, "CanSeal", v.can_seal);
    Get(j, "CanStore", v.can_store);
  }

  JSON_ENCODE(WorkerResources) {
    Value j{rapidjson::kObjectType};
    Set(j, "MemPhysical", v.physical_memory, allocator);
    Set(j, "MemSwap", v.swap_memory, allocator);
    Set(j, "MemUsed", v.reserved_memory, allocator);
    Set(j, "CPUs", v.cpus, allocator);
    Set(j, "GPUs", v.gpus, allocator);
    return j;
  }

  JSON_DECODE(WorkerResources) {
    Get(j, "MemPhysical", v.physical_memory);
    Get(j, "MemSwap", v.swap_memory);
    Get(j, "MemUsed", v.reserved_memory);
    Get(j, "CPUs", v.cpus);
    Get(j, "GPUs", v.gpus);
  }

  JSON_ENCODE(WorkerInfo) {
    Value j{rapidjson::kObjectType};
    Set(j, "Hostname", v.hostname, allocator);
    Set(j, "Resources", v.resources, allocator);
    return j;
  }

  JSON_DECODE(WorkerInfo) {
    Get(j, "Hostname", v.hostname);
    Get(j, "Resources", v.resources);
  }

}  // namespace fc::primitives
