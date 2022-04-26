/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "codec/json/coding.hpp"

#include "sector_storage/stores/storage.hpp"

namespace fc::sector_storage::stores {
  using codec::json::Get;
  using codec::json::Set;
  using codec::json::Value;

  JSON_ENCODE(LocalPath) {
    Value j{rapidjson::kObjectType};
    Set(j, "Path", v.path, allocator);
    return j;
  }

  JSON_DECODE(LocalPath) {
    Get(j, "Path", v.path);
  }

  JSON_ENCODE(StorageConfig) {
    Value j{rapidjson::kObjectType};
    Set(j, "StoragePaths", v.storage_paths, allocator);
    return j;
  }

  JSON_DECODE(StorageConfig) {
    Get(j, "StoragePaths", v.storage_paths);
  }

}  // namespace fc::sector_storage::stores
