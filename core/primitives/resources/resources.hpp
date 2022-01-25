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

  const std::map<std::pair<TaskType, RegisteredSealProof>, Resources>
      kResourceTable = {
          // Add Piece
          {std::pair(kTTAddPiece, RegisteredSealProof::kStackedDrg64GiBV1),
           Resources{
               .min_memory = uint64_t(64) << 30,
               .max_memory = uint64_t(64) << 30,
               .threads = 1,
               .can_gpu = false,
               .base_min_memory = 1 << 30,
           }},
          {std::pair(kTTAddPiece, RegisteredSealProof::kStackedDrg32GiBV1),
           Resources{
               .min_memory = uint64_t(64) << 30,
               .max_memory = uint64_t(64) << 30,
               .threads = 1,
               .can_gpu = false,
               .base_min_memory = 1 << 30,
           }},
          {std::pair(kTTAddPiece, RegisteredSealProof::kStackedDrg512MiBV1),
           Resources{
               .min_memory = 1 << 30,
               .max_memory = 1 << 30,
               .threads = 1,
               .can_gpu = false,
               .base_min_memory = 1 << 30,
           }},
          {std::pair(kTTAddPiece, RegisteredSealProof::kStackedDrg8MiBV1),
           Resources{
               .min_memory = 8 << 20,
               .max_memory = 8 << 20,
               .threads = 1,
               .can_gpu = false,
               .base_min_memory = 8 << 20,
           }},
          {std::pair(kTTAddPiece, RegisteredSealProof::kStackedDrg2KiBV1),
           Resources{
               .min_memory = 2 << 10,
               .max_memory = 2 << 10,
               .threads = 1,
               .can_gpu = false,
               .base_min_memory = 2 << 10,
           }},

          // PreCommit1
          {std::pair(kTTPreCommit1, RegisteredSealProof::kStackedDrg64GiBV1),
           Resources{
               .min_memory = uint64_t(112) << 30,
               .max_memory = uint64_t(128) << 30,
               .threads = 1,
               .can_gpu = false,
               .base_min_memory = 10 << 20,
           }},
          {std::pair(kTTPreCommit1, RegisteredSealProof::kStackedDrg32GiBV1),
           Resources{
               .min_memory = uint64_t(56) << 30,
               .max_memory = uint64_t(64) << 30,
               .threads = 1,
               .can_gpu = false,
               .base_min_memory = 1 << 30,
           }},
          {std::pair(kTTPreCommit1, RegisteredSealProof::kStackedDrg512MiBV1),
           Resources{
               .min_memory = 768 << 20,
               .max_memory = 1 << 30,
               .threads = 1,
               .can_gpu = false,
               .base_min_memory = 1 << 20,
           }},
          {std::pair(kTTPreCommit1, RegisteredSealProof::kStackedDrg8MiBV1),
           Resources{
               .min_memory = 8 << 20,
               .max_memory = 8 << 20,
               .threads = 1,
               .can_gpu = false,
               .base_min_memory = 8 << 20,
           }},
          {std::pair(kTTPreCommit1, RegisteredSealProof::kStackedDrg2KiBV1),
           Resources{
               .min_memory = 2 << 10,
               .max_memory = 2 << 10,
               .threads = 1,
               .can_gpu = false,
               .base_min_memory = 2 << 10,
           }},

          // PreCommit2
          {std::pair(kTTPreCommit2, RegisteredSealProof::kStackedDrg64GiBV1),
           Resources{
               .min_memory = uint64_t(64) << 30,
               .max_memory = uint64_t(64) << 30,
               .threads = boost::none,
               .can_gpu = true,
               .base_min_memory = uint64_t(60) << 30,
           }},
          {std::pair(kTTPreCommit2, RegisteredSealProof::kStackedDrg32GiBV1),
           Resources{
               .min_memory = uint64_t(32) << 30,
               .max_memory = uint64_t(32) << 30,
               .threads = boost::none,
               .can_gpu = true,
               .base_min_memory = uint64_t(30) << 30,
           }},
          {std::pair(kTTPreCommit2, RegisteredSealProof::kStackedDrg512MiBV1),
           Resources{
               .min_memory = uint64_t(1) << 30,
               .max_memory = uint64_t(3) << 29,
               .threads = boost::none,
               .can_gpu = false,
               .base_min_memory = uint64_t(1) << 30,
           }},
          {std::pair(kTTPreCommit2, RegisteredSealProof::kStackedDrg8MiBV1),
           Resources{
               .min_memory = uint64_t(8) << 20,
               .max_memory = uint64_t(8) << 20,
               .threads = boost::none,
               .can_gpu = false,
               .base_min_memory = uint64_t(8) << 20,
           }},
          {std::pair(kTTPreCommit2, RegisteredSealProof::kStackedDrg2KiBV1),
           Resources{
               .min_memory = uint64_t(2) << 10,
               .max_memory = uint64_t(2) << 10,
               .threads = boost::none,
               .can_gpu = false,
               .base_min_memory = uint64_t(2) << 10,
           }},

          // Commit1
          {std::pair(kTTCommit1, RegisteredSealProof::kStackedDrg64GiBV1),
           Resources{
               .min_memory = uint64_t(1) << 30,
               .max_memory = uint64_t(1) << 30,
               .threads = 0,
               .can_gpu = false,
               .base_min_memory = uint64_t(1) << 30,
           }},
          {std::pair(kTTCommit1, RegisteredSealProof::kStackedDrg32GiBV1),
           Resources{
               .min_memory = uint64_t(1) << 30,
               .max_memory = uint64_t(1) << 30,
               .threads = 0,
               .can_gpu = false,
               .base_min_memory = uint64_t(1) << 30,
           }},
          {std::pair(kTTCommit1, RegisteredSealProof::kStackedDrg512MiBV1),
           Resources{
               .min_memory = uint64_t(1) << 30,
               .max_memory = uint64_t(1) << 30,
               .threads = 0,
               .can_gpu = false,
               .base_min_memory = uint64_t(1) << 30,
           }},
          {std::pair(kTTCommit1, RegisteredSealProof::kStackedDrg8MiBV1),
           Resources{
               .min_memory = uint64_t(8) << 20,
               .max_memory = uint64_t(8) << 20,
               .threads = 0,
               .can_gpu = false,
               .base_min_memory = uint64_t(8) << 20,
           }},
          {std::pair(kTTCommit1, RegisteredSealProof::kStackedDrg2KiBV1),
           Resources{
               .min_memory = uint64_t(2) << 10,
               .max_memory = uint64_t(2) << 10,
               .threads = 0,
               .can_gpu = false,
               .base_min_memory = uint64_t(2) << 10,
           }},

          // Commit2
          {std::pair(kTTCommit2, RegisteredSealProof::kStackedDrg64GiBV1),
           Resources{
               .min_memory = uint64_t(60) << 30,
               .max_memory = uint64_t(190) << 30,
               .threads = boost::none,
               .can_gpu = true,
               .base_min_memory = uint64_t(64) << 30,
           }},
          {std::pair(kTTCommit2, RegisteredSealProof::kStackedDrg32GiBV1),
           Resources{
               .min_memory = uint64_t(30) << 30,
               .max_memory = uint64_t(150) << 30,
               .threads = boost::none,
               .can_gpu = true,
               .base_min_memory = uint64_t(32) << 30,
           }},
          {std::pair(kTTCommit2, RegisteredSealProof::kStackedDrg512MiBV1),
           Resources{
               .min_memory = 1 << 30,
               .max_memory = uint64_t(3) << 29,
               .threads = 1,
               .can_gpu = true,
               .base_min_memory = uint64_t(10) << 30,
           }},
          {std::pair(kTTCommit2, RegisteredSealProof::kStackedDrg8MiBV1),
           Resources{
               .min_memory = uint64_t(8) << 20,
               .max_memory = uint64_t(8) << 20,
               .threads = 1,
               .can_gpu = true,
               .base_min_memory = uint64_t(8) << 20,
           }},
          {std::pair(kTTCommit2, RegisteredSealProof::kStackedDrg2KiBV1),
           Resources{
               .min_memory = uint64_t(2) << 10,
               .max_memory = uint64_t(2) << 10,
               .threads = 1,
               .can_gpu = true,
               .base_min_memory = uint64_t(2) << 10,
           }},

          // Fetch
          {std::pair(kTTFetch, RegisteredSealProof::kStackedDrg64GiBV1),
           Resources{
               .min_memory = uint64_t(1) << 20,
               .max_memory = uint64_t(1) << 20,
               .threads = 0,
               .can_gpu = false,
               .base_min_memory = 0,
           }},
          {std::pair(kTTFetch, RegisteredSealProof::kStackedDrg32GiBV1),
           Resources{
               .min_memory = uint64_t(1) << 20,
               .max_memory = uint64_t(1) << 20,
               .threads = 0,
               .can_gpu = false,
               .base_min_memory = 0,
           }},
          {std::pair(kTTFetch, RegisteredSealProof::kStackedDrg512MiBV1),
           Resources{
               .min_memory = uint64_t(1) << 20,
               .max_memory = uint64_t(1) << 20,
               .threads = 0,
               .can_gpu = false,
               .base_min_memory = 0,
           }},
          {std::pair(kTTFetch, RegisteredSealProof::kStackedDrg8MiBV1),
           Resources{
               .min_memory = uint64_t(1) << 20,
               .max_memory = uint64_t(1) << 20,
               .threads = 0,
               .can_gpu = false,
               .base_min_memory = 0,
           }},
          {std::pair(kTTFetch, RegisteredSealProof::kStackedDrg2KiBV1),
           Resources{
               .min_memory = uint64_t(1) << 20,
               .max_memory = uint64_t(1) << 20,
               .threads = 0,
               .can_gpu = false,
               .base_min_memory = 0,
           }},

          // ReadUnseal - is is should be same as Fetch
          {std::pair(kTTReadUnsealed, RegisteredSealProof::kStackedDrg64GiBV1),
           Resources{
               .min_memory = uint64_t(1) << 20,
               .max_memory = uint64_t(1) << 20,
               .threads = 0,
               .can_gpu = false,
               .base_min_memory = 0,
           }},
          {std::pair(kTTReadUnsealed, RegisteredSealProof::kStackedDrg32GiBV1),
           Resources{
               .min_memory = uint64_t(1) << 20,
               .max_memory = uint64_t(1) << 20,
               .threads = 0,
               .can_gpu = false,
               .base_min_memory = 0,
           }},
          {std::pair(kTTReadUnsealed, RegisteredSealProof::kStackedDrg512MiBV1),
           Resources{
               .min_memory = uint64_t(1) << 20,
               .max_memory = uint64_t(1) << 20,
               .threads = 0,
               .can_gpu = false,
               .base_min_memory = 0,
           }},
          {std::pair(kTTReadUnsealed, RegisteredSealProof::kStackedDrg8MiBV1),
           Resources{
               .min_memory = uint64_t(1) << 20,
               .max_memory = uint64_t(1) << 20,
               .threads = 0,
               .can_gpu = false,
               .base_min_memory = 0,
           }},
          {std::pair(kTTReadUnsealed, RegisteredSealProof::kStackedDrg2KiBV1),
           Resources{
               .min_memory = uint64_t(1) << 20,
               .max_memory = uint64_t(1) << 20,
               .threads = 0,
               .can_gpu = false,
               .base_min_memory = 0,
           }},

          // Unseal - is is should be same as PreCommit1
          {std::pair(kTTUnseal, RegisteredSealProof::kStackedDrg64GiBV1),
           Resources{
               .min_memory = uint64_t(112) << 30,
               .max_memory = uint64_t(128) << 30,
               .threads = 1,
               .can_gpu = false,
               .base_min_memory = uint64_t(10) << 20,
           }},
          {std::pair(kTTUnseal, RegisteredSealProof::kStackedDrg32GiBV1),
           Resources{
               .min_memory = uint64_t(56) << 30,
               .max_memory = uint64_t(64) << 30,
               .threads = 1,
               .can_gpu = false,
               .base_min_memory = uint64_t(1) << 30,
           }},
          {std::pair(kTTUnseal, RegisteredSealProof::kStackedDrg512MiBV1),
           Resources{
               .min_memory = uint64_t(768) << 20,
               .max_memory = uint64_t(1) << 30,
               .threads = 1,
               .can_gpu = false,
               .base_min_memory = uint64_t(1) << 20,
           }},
          {std::pair(kTTUnseal, RegisteredSealProof::kStackedDrg8MiBV1),
           Resources{
               .min_memory = uint64_t(8) << 20,
               .max_memory = uint64_t(8) << 20,
               .threads = 1,
               .can_gpu = false,
               .base_min_memory = uint64_t(8) << 20,
           }},
          {std::pair(kTTUnseal, RegisteredSealProof::kStackedDrg2KiBV1),
           Resources{
               .min_memory = uint64_t(2) << 10,
               .max_memory = uint64_t(2) << 10,
               .threads = 1,
               .can_gpu = false,
               .base_min_memory = uint64_t(2) << 10,
           }},
  };

}  // namespace fc::primitives
