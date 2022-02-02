/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/optional.hpp>
#include <map>
#include <primitives/sector/sector.hpp>
#include "primitives/seal_tasks/task.hpp"
#include "primitives/sector/sector.hpp"

namespace fc::primitives {
  using sector::RegisteredSealProof;

  struct Resources {
    uint64_t min_memory{};  // What Must be in RAM for decent perf
    uint64_t max_memory{};  // Memory required (swap + ram)

    boost::optional<uint64_t> threads = 0;  // multithread = boost::none

    bool can_gpu = false;

    uint64_t base_min_memory{};  // What Must be in RAM for decent perf (shared
                                 // between threads)
  };

  inline bool operator==(const Resources &lhs, const Resources &rhs) {
    return lhs.min_memory == rhs.min_memory && lhs.max_memory == rhs.max_memory
           && lhs.threads == rhs.threads && lhs.can_gpu == rhs.can_gpu
           && lhs.base_min_memory == rhs.base_min_memory;
  }

  using ResourceTable =
      std::map<TaskType, std::map<RegisteredSealProof, Resources>>;

  inline const ResourceTable &getResourceTable() {
    static const ResourceTable kResourceTable = [] {
      ResourceTable res = {
          {kTTAddPiece,
           {
               {RegisteredSealProof::kStackedDrg64GiBV1,
                Resources{
                    .min_memory = uint64_t(8) << 30,
                    .max_memory = uint64_t(8) << 30,
                    .threads = 1,
                    .can_gpu = false,
                    .base_min_memory = uint64_t(1) << 30,
                }},
               {RegisteredSealProof::kStackedDrg32GiBV1,
                Resources{
                    .min_memory = uint64_t(4) << 30,
                    .max_memory = uint64_t(4) << 30,
                    .threads = 1,
                    .can_gpu = false,
                    .base_min_memory = uint64_t(1) << 30,
                }},
               {RegisteredSealProof::kStackedDrg512MiBV1,
                Resources{
                    .min_memory = uint64_t(1) << 30,
                    .max_memory = uint64_t(1) << 30,
                    .threads = 1,
                    .can_gpu = false,
                    .base_min_memory = uint64_t(1) << 30,
                }},
               {RegisteredSealProof::kStackedDrg8MiBV1,
                Resources{
                    .min_memory = uint64_t(8) << 20,
                    .max_memory = uint64_t(8) << 20,
                    .threads = 1,
                    .can_gpu = false,
                    .base_min_memory = uint64_t(8) << 20,
                }},
               {RegisteredSealProof::kStackedDrg2KiBV1,
                Resources{
                    .min_memory = uint64_t(2) << 10,
                    .max_memory = uint64_t(2) << 10,
                    .threads = 1,
                    .can_gpu = false,
                    .base_min_memory = uint64_t(2) << 10,
                }},
           }},
          {kTTPreCommit1,
           {
               {RegisteredSealProof::kStackedDrg64GiBV1,
                Resources{
                    .min_memory = uint64_t(112) << 30,
                    .max_memory = uint64_t(128) << 30,
                    .threads = 1,
                    .can_gpu = false,
                    .base_min_memory = uint64_t(10) << 20,
                }},
               {RegisteredSealProof::kStackedDrg32GiBV1,
                Resources{
                    .min_memory = uint64_t(56) << 30,
                    .max_memory = uint64_t(64) << 30,
                    .threads = 1,
                    .can_gpu = false,
                    .base_min_memory = uint64_t(10) << 20,
                }},
               {RegisteredSealProof::kStackedDrg512MiBV1,
                Resources{
                    .min_memory = uint64_t(768) << 20,
                    .max_memory = uint64_t(1) << 30,
                    .threads = 1,
                    .can_gpu = false,
                    .base_min_memory = uint64_t(1) << 20,
                }},
               {RegisteredSealProof::kStackedDrg8MiBV1,
                Resources{
                    .min_memory = uint64_t(8) << 20,
                    .max_memory = uint64_t(8) << 20,
                    .threads = 1,
                    .can_gpu = false,
                    .base_min_memory = uint64_t(8) << 20,
                }},
               {RegisteredSealProof::kStackedDrg2KiBV1,
                Resources{
                    .min_memory = uint64_t(2) << 10,
                    .max_memory = uint64_t(2) << 10,
                    .threads = 1,
                    .can_gpu = false,
                    .base_min_memory = uint64_t(2) << 10,
                }},
           }},
          {kTTPreCommit2,
           {
               {RegisteredSealProof::kStackedDrg64GiBV1,
                Resources{
                    .min_memory = uint64_t(30) << 30,
                    .max_memory = uint64_t(30) << 30,
                    .threads = boost::none,
                    .can_gpu = true,
                    .base_min_memory = uint64_t(1) << 30,
                }},
               {RegisteredSealProof::kStackedDrg32GiBV1,
                Resources{
                    .min_memory = uint64_t(15) << 30,
                    .max_memory = uint64_t(15) << 30,
                    .threads = boost::none,
                    .can_gpu = true,
                    .base_min_memory = uint64_t(1) << 30,
                }},
               {RegisteredSealProof::kStackedDrg512MiBV1,
                Resources{
                    .min_memory = uint64_t(1) << 30,
                    .max_memory = uint64_t(3) << 29,
                    .threads = boost::none,
                    .can_gpu = false,
                    .base_min_memory = uint64_t(1) << 30,
                }},
               {RegisteredSealProof::kStackedDrg8MiBV1,
                Resources{
                    .min_memory = uint64_t(8) << 20,
                    .max_memory = uint64_t(8) << 20,
                    .threads = boost::none,
                    .can_gpu = false,
                    .base_min_memory = uint64_t(8) << 20,
                }},
               {RegisteredSealProof::kStackedDrg2KiBV1,
                Resources{
                    .min_memory = uint64_t(2) << 10,
                    .max_memory = uint64_t(2) << 10,
                    .threads = boost::none,
                    .can_gpu = false,
                    .base_min_memory = uint64_t(2) << 10,
                }},
           }},
          {kTTCommit1,
           {
               {RegisteredSealProof::kStackedDrg64GiBV1,
                Resources{
                    .min_memory = uint64_t(1) << 30,
                    .max_memory = uint64_t(1) << 30,
                    .threads = 0,
                    .can_gpu = false,
                    .base_min_memory = uint64_t(1) << 30,
                }},
               {RegisteredSealProof::kStackedDrg32GiBV1,
                Resources{
                    .min_memory = uint64_t(1) << 30,
                    .max_memory = uint64_t(1) << 30,
                    .threads = 0,
                    .can_gpu = false,
                    .base_min_memory = uint64_t(1) << 30,
                }},
               {RegisteredSealProof::kStackedDrg512MiBV1,
                Resources{
                    .min_memory = uint64_t(1) << 30,
                    .max_memory = uint64_t(1) << 30,
                    .threads = 0,
                    .can_gpu = false,
                    .base_min_memory = uint64_t(1) << 30,
                }},
               {RegisteredSealProof::kStackedDrg8MiBV1,
                Resources{
                    .min_memory = uint64_t(8) << 20,
                    .max_memory = uint64_t(8) << 20,
                    .threads = 0,
                    .can_gpu = false,
                    .base_min_memory = uint64_t(8) << 20,
                }},
               {RegisteredSealProof::kStackedDrg2KiBV1,
                Resources{
                    .min_memory = uint64_t(2) << 10,
                    .max_memory = uint64_t(2) << 10,
                    .threads = 0,
                    .can_gpu = false,
                    .base_min_memory = uint64_t(2) << 10,
                }},
           }},
          {kTTCommit2,
           {
               {RegisteredSealProof::kStackedDrg64GiBV1,
                Resources{
                    .min_memory = uint64_t(60) << 30,
                    .max_memory = uint64_t(190) << 30,
                    .threads = boost::none,
                    .can_gpu = true,
                    .base_min_memory = uint64_t(64) << 30,
                }},
               {RegisteredSealProof::kStackedDrg32GiBV1,
                Resources{
                    .min_memory = uint64_t(30) << 30,
                    .max_memory = uint64_t(150) << 30,
                    .threads = boost::none,
                    .can_gpu = true,
                    .base_min_memory = uint64_t(32) << 30,
                }},
               {RegisteredSealProof::kStackedDrg512MiBV1,
                Resources{
                    .min_memory = uint64_t(1) << 30,
                    .max_memory = uint64_t(3) << 29,  // 1.5G
                    .threads = 1,
                    .can_gpu = true,
                    .base_min_memory = uint64_t(10) << 30,
                }},
               {RegisteredSealProof::kStackedDrg8MiBV1,
                Resources{
                    .min_memory = uint64_t(8) << 20,
                    .max_memory = uint64_t(8) << 20,
                    .threads = 1,
                    .can_gpu = true,
                    .base_min_memory = uint64_t(8) << 20,
                }},
               {RegisteredSealProof::kStackedDrg2KiBV1,
                Resources{
                    .min_memory = uint64_t(2) << 10,
                    .max_memory = uint64_t(2) << 10,
                    .threads = 1,
                    .can_gpu = true,
                    .base_min_memory = uint64_t(2) << 10,
                }},
           }},
          {kTTFetch,
           {
               {RegisteredSealProof::kStackedDrg64GiBV1,
                Resources{
                    .min_memory = uint64_t(1) << 20,
                    .max_memory = uint64_t(1) << 20,
                    .threads = 0,
                    .can_gpu = false,
                    .base_min_memory = 0,
                }},
               {RegisteredSealProof::kStackedDrg32GiBV1,
                Resources{
                    .min_memory = uint64_t(1) << 20,
                    .max_memory = uint64_t(1) << 20,
                    .threads = 0,
                    .can_gpu = false,
                    .base_min_memory = 0,
                }},
               {RegisteredSealProof::kStackedDrg512MiBV1,
                Resources{
                    .min_memory = uint64_t(1) << 20,
                    .max_memory = uint64_t(1) << 20,
                    .threads = 0,
                    .can_gpu = false,
                    .base_min_memory = 0,
                }},
               {RegisteredSealProof::kStackedDrg8MiBV1,
                Resources{
                    .min_memory = uint64_t(1) << 20,
                    .max_memory = uint64_t(1) << 20,
                    .threads = 0,
                    .can_gpu = false,
                    .base_min_memory = 0,
                }},
               {RegisteredSealProof::kStackedDrg2KiBV1,
                Resources{
                    .min_memory = uint64_t(1) << 20,
                    .max_memory = uint64_t(1) << 20,
                    .threads = 0,
                    .can_gpu = false,
                    .base_min_memory = 0,
                }},
           }},
      };

      res[kTTUnseal] = res[kTTPreCommit1];
      res[kTTReadUnsealed] = res[kTTFetch];
      res[kTTReplicaUpdate] = res[kTTAddPiece];
      res[kTTProveReplicaUpdate1] = res[kTTCommit1];
      res[kTTProveReplicaUpdate2] = res[kTTCommit2];
      res[kTTRegenSectorKey] = res[kTTReplicaUpdate];

      for (auto &[key, value] : res) {
        value[RegisteredSealProof::kStackedDrg2KiBV1_1] =
            value[RegisteredSealProof::kStackedDrg2KiBV1];
        value[RegisteredSealProof::kStackedDrg8MiBV1_1] =
            value[RegisteredSealProof::kStackedDrg8MiBV1];
        value[RegisteredSealProof::kStackedDrg512MiBV1_1] =
            value[RegisteredSealProof::kStackedDrg512MiBV1];
        value[RegisteredSealProof::kStackedDrg32GiBV1_1] =
            value[RegisteredSealProof::kStackedDrg32GiBV1];
        value[RegisteredSealProof::kStackedDrg64GiBV1_1] =
            value[RegisteredSealProof::kStackedDrg64GiBV1];
      }

      return res;
    }();

    return kResourceTable;
  }

}  // namespace fc::primitives
