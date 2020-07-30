/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_PRIMITIVES_RESOURCES_RESOURCES_HPP
#define CPP_FILECOIN_CORE_PRIMITIVES_RESOURCES_RESOURCES_HPP

#include <boost/optional.hpp>
#include <map>
#include "primitives/seal_tasks/task.hpp"
#include "primitives/sector/sector.hpp"

namespace fc::primitives {

  using fc::primitives::sector::RegisteredProof;

  struct Resources {
    uint64_t min_memory;  // What Must be in RAM for decent perf
    uint64_t max_memory;  // Memory required (swap + ram)

    boost::optional<uint64_t> threads;  // multithread = boost::none

    bool can_gpu;

    uint64_t base_min_memory;  // What Must be in RAM for decent perf (shared
                               // between threads)
  };

  inline bool operator==(const Resources &lhs, const Resources &rhs) {
    return lhs.min_memory == rhs.min_memory && lhs.max_memory == rhs.max_memory
           && lhs.threads == rhs.threads && lhs.can_gpu == rhs.can_gpu
           && lhs.base_min_memory == rhs.base_min_memory;
  }

  const std::map<std::pair<TaskType, RegisteredProof>, Resources>
      kResourceTable = {
          // Add Piece
          {std::pair(kTTAddPiece, RegisteredProof::StackedDRG64GiBSeal),
           Resources{
               .min_memory = uint64_t(64) << 30,
               .max_memory = uint64_t(64) << 30,
               .threads = 1,
               .can_gpu = false,
               .base_min_memory = 1 << 30,
           }},
          {std::pair(kTTAddPiece, RegisteredProof::StackedDRG32GiBSeal),
           Resources{
               .min_memory = uint64_t(64) << 30,
               .max_memory = uint64_t(64) << 30,
               .threads = 1,
               .can_gpu = false,
               .base_min_memory = 1 << 30,
           }},
          {std::pair(kTTAddPiece, RegisteredProof::StackedDRG512MiBSeal),
           Resources{
               .min_memory = 1 << 30,
               .max_memory = 1 << 30,
               .threads = 1,
               .can_gpu = false,
               .base_min_memory = 1 << 30,
           }},
          {std::pair(kTTAddPiece, RegisteredProof::StackedDRG8MiBSeal),
           Resources{
               .min_memory = 8 << 20,
               .max_memory = 8 << 20,
               .threads = 1,
               .can_gpu = false,
               .base_min_memory = 8 << 20,
           }},
          {std::pair(kTTAddPiece, RegisteredProof::StackedDRG2KiBSeal),
           Resources{
               .min_memory = 2 << 10,
               .max_memory = 2 << 10,
               .threads = 1,
               .can_gpu = false,
               .base_min_memory = 2 << 10,
           }},

          // PreCommit1
          {std::pair(kTTPreCommit1, RegisteredProof::StackedDRG64GiBSeal),
           Resources{
               .min_memory = uint64_t(112) << 30,
               .max_memory = uint64_t(128) << 30,
               .threads = 1,
               .can_gpu = false,
               .base_min_memory = 10 << 20,
           }},
          {std::pair(kTTPreCommit1, RegisteredProof::StackedDRG32GiBSeal),
           Resources{
               .min_memory = uint64_t(56) << 30,
               .max_memory = uint64_t(64) << 30,
               .threads = 1,
               .can_gpu = false,
               .base_min_memory = 1 << 30,
           }},
          {std::pair(kTTPreCommit1, RegisteredProof::StackedDRG512MiBSeal),
           Resources{
               .min_memory = 768 << 20,
               .max_memory = 1 << 30,
               .threads = 1,
               .can_gpu = false,
               .base_min_memory = 1 << 20,
           }},
          {std::pair(kTTPreCommit1, RegisteredProof::StackedDRG8MiBSeal),
           Resources{
               .min_memory = 8 << 20,
               .max_memory = 8 << 20,
               .threads = 1,
               .can_gpu = false,
               .base_min_memory = 8 << 20,
           }},
          {std::pair(kTTPreCommit1, RegisteredProof::StackedDRG2KiBSeal),
           Resources{
               .min_memory = 2 << 10,
               .max_memory = 2 << 10,
               .threads = 1,
               .can_gpu = false,
               .base_min_memory = 2 << 10,
           }},

          // PreCommit2
          {std::pair(kTTPreCommit2, RegisteredProof::StackedDRG64GiBSeal),
           Resources{
               .min_memory = uint64_t(64) << 30,
               .max_memory = uint64_t(64) << 30,
               .threads = -1,
               .can_gpu = true,
               .base_min_memory = uint64_t(60) << 30,
           }},
          {std::pair(kTTPreCommit2, RegisteredProof::StackedDRG32GiBSeal),
           Resources{
               .min_memory = uint64_t(32) << 30,
               .max_memory = uint64_t(32) << 30,
               .threads = -1,
               .can_gpu = true,
               .base_min_memory = uint64_t(30) << 30,
           }},
          {std::pair(kTTPreCommit2, RegisteredProof::StackedDRG512MiBSeal),
           Resources{
               .min_memory = uint64_t(1) << 30,
               .max_memory = uint64_t(3) << 29,
               .threads = -1,
               .can_gpu = false,
               .base_min_memory = uint64_t(1) << 30,
           }},
          {std::pair(kTTPreCommit2, RegisteredProof::StackedDRG8MiBSeal),
           Resources{
               .min_memory = uint64_t(8) << 20,
               .max_memory = uint64_t(8) << 20,
               .threads = -1,
               .can_gpu = false,
               .base_min_memory = uint64_t(8) << 20,
           }},
          {std::pair(kTTPreCommit2, RegisteredProof::StackedDRG2KiBSeal),
           Resources{
               .min_memory = uint64_t(2) << 10,
               .max_memory = uint64_t(2) << 10,
               .threads = -1,
               .can_gpu = false,
               .base_min_memory = uint64_t(2) << 10,
           }},

          // Commit1
          {std::pair(kTTCommit1, RegisteredProof::StackedDRG64GiBSeal),
           Resources{
               .min_memory = uint64_t(1) << 30,
               .max_memory = uint64_t(1) << 30,
               .threads = 0,
               .can_gpu = false,
               .base_min_memory = uint64_t(1) << 30,
           }},
          {std::pair(kTTCommit1, RegisteredProof::StackedDRG32GiBSeal),
           Resources{
               .min_memory = uint64_t(1) << 30,
               .max_memory = uint64_t(1) << 30,
               .threads = 0,
               .can_gpu = false,
               .base_min_memory = uint64_t(1) << 30,
           }},
          {std::pair(kTTCommit1, RegisteredProof::StackedDRG512MiBSeal),
           Resources{
               .min_memory = uint64_t(1) << 30,
               .max_memory = uint64_t(1) << 30,
               .threads = 0,
               .can_gpu = false,
               .base_min_memory = uint64_t(1) << 30,
           }},
          {std::pair(kTTCommit1, RegisteredProof::StackedDRG8MiBSeal),
           Resources{
               .min_memory = uint64_t(8) << 20,
               .max_memory = uint64_t(8) << 20,
               .threads = 0,
               .can_gpu = false,
               .base_min_memory = uint64_t(8) << 20,
           }},
          {std::pair(kTTCommit1, RegisteredProof::StackedDRG2KiBSeal),
           Resources{
               .min_memory = uint64_t(2) << 10,
               .max_memory = uint64_t(2) << 10,
               .threads = 0,
               .can_gpu = false,
               .base_min_memory = uint64_t(2) << 10,
           }},

          // Commit2
          {std::pair(kTTCommit2, RegisteredProof::StackedDRG64GiBSeal),
           Resources{
               .min_memory = uint64_t(60) << 30,
               .max_memory = uint64_t(190) << 30,
               .threads = -1,
               .can_gpu = true,
               .base_min_memory = uint64_t(64) << 30,
           }},
          {std::pair(kTTCommit2, RegisteredProof::StackedDRG32GiBSeal),
           Resources{
               .min_memory = uint64_t(30) << 30,
               .max_memory = uint64_t(150) << 30,
               .threads = -1,
               .can_gpu = true,
               .base_min_memory = uint64_t(32) << 30,
           }},
          {std::pair(kTTCommit2, RegisteredProof::StackedDRG512MiBSeal),
           Resources{
               .min_memory = 1 << 30,
               .max_memory = uint64_t(3) << 29,
               .threads = 1,
               .can_gpu = true,
               .base_min_memory = uint64_t(10) << 30,
           }},
          {std::pair(kTTCommit2, RegisteredProof::StackedDRG8MiBSeal),
           Resources{
               .min_memory = uint64_t(8) << 20,
               .max_memory = uint64_t(8) << 20,
               .threads = 1,
               .can_gpu = true,
               .base_min_memory = uint64_t(8) << 20,
           }},
          {std::pair(kTTCommit2, RegisteredProof::StackedDRG2KiBSeal),
           Resources{
               .min_memory = uint64_t(2) << 10,
               .max_memory = uint64_t(2) << 10,
               .threads = 1,
               .can_gpu = true,
               .base_min_memory = uint64_t(2) << 10,
           }},

          // Fetch
          {std::pair(kTTFetch, RegisteredProof::StackedDRG64GiBSeal),
           Resources{
               .min_memory = uint64_t(1) << 20,
               .max_memory = uint64_t(1) << 20,
               .threads = 0,
               .can_gpu = false,
               .base_min_memory = 0,
           }},
          {std::pair(kTTFetch, RegisteredProof::StackedDRG32GiBSeal),
           Resources{
               .min_memory = uint64_t(1) << 20,
               .max_memory = uint64_t(1) << 20,
               .threads = 0,
               .can_gpu = false,
               .base_min_memory = 0,
           }},
          {std::pair(kTTFetch, RegisteredProof::StackedDRG512MiBSeal),
           Resources{
               .min_memory = uint64_t(1) << 20,
               .max_memory = uint64_t(1) << 20,
               .threads = 0,
               .can_gpu = false,
               .base_min_memory = 0,
           }},
          {std::pair(kTTFetch, RegisteredProof::StackedDRG8MiBSeal),
           Resources{
               .min_memory = uint64_t(1) << 20,
               .max_memory = uint64_t(1) << 20,
               .threads = 0,
               .can_gpu = false,
               .base_min_memory = 0,
           }},
          {std::pair(kTTFetch, RegisteredProof::StackedDRG2KiBSeal),
           Resources{
               .min_memory = uint64_t(1) << 20,
               .max_memory = uint64_t(1) << 20,
               .threads = 0,
               .can_gpu = false,
               .base_min_memory = 0,
           }},

          // ReadUnseal - is is should be same as Fetch
          {std::pair(kTTReadUnsealed, RegisteredProof::StackedDRG64GiBSeal),
           Resources{
               .min_memory = uint64_t(1) << 20,
               .max_memory = uint64_t(1) << 20,
               .threads = 0,
               .can_gpu = false,
               .base_min_memory = 0,
           }},
          {std::pair(kTTReadUnsealed, RegisteredProof::StackedDRG32GiBSeal),
           Resources{
               .min_memory = uint64_t(1) << 20,
               .max_memory = uint64_t(1) << 20,
               .threads = 0,
               .can_gpu = false,
               .base_min_memory = 0,
           }},
          {std::pair(kTTReadUnsealed, RegisteredProof::StackedDRG512MiBSeal),
           Resources{
               .min_memory = uint64_t(1) << 20,
               .max_memory = uint64_t(1) << 20,
               .threads = 0,
               .can_gpu = false,
               .base_min_memory = 0,
           }},
          {std::pair(kTTReadUnsealed, RegisteredProof::StackedDRG8MiBSeal),
           Resources{
               .min_memory = uint64_t(1) << 20,
               .max_memory = uint64_t(1) << 20,
               .threads = 0,
               .can_gpu = false,
               .base_min_memory = 0,
           }},
          {std::pair(kTTReadUnsealed, RegisteredProof::StackedDRG2KiBSeal),
           Resources{
               .min_memory = uint64_t(1) << 20,
               .max_memory = uint64_t(1) << 20,
               .threads = 0,
               .can_gpu = false,
               .base_min_memory = 0,
           }},

          // Unseal - is is should be same as PreCommit1
          {std::pair(kTTUnseal, RegisteredProof::StackedDRG64GiBSeal),
           Resources{
               .min_memory = uint64_t(112) << 30,
               .max_memory = uint64_t(128) << 30,
               .threads = 1,
               .can_gpu = false,
               .base_min_memory = uint64_t(10) << 20,
           }},
          {std::pair(kTTUnseal, RegisteredProof::StackedDRG32GiBSeal),
           Resources{
               .min_memory = uint64_t(56) << 30,
               .max_memory = uint64_t(64) << 30,
               .threads = 1,
               .can_gpu = false,
               .base_min_memory = uint64_t(1) << 30,
           }},
          {std::pair(kTTUnseal, RegisteredProof::StackedDRG512MiBSeal),
           Resources{
               .min_memory = uint64_t(768) << 20,
               .max_memory = uint64_t(1) << 30,
               .threads = 1,
               .can_gpu = false,
               .base_min_memory = uint64_t(1) << 20,
           }},
          {std::pair(kTTUnseal, RegisteredProof::StackedDRG8MiBSeal),
           Resources{
               .min_memory = uint64_t(8) << 20,
               .max_memory = uint64_t(8) << 20,
               .threads = 1,
               .can_gpu = false,
               .base_min_memory = uint64_t(8) << 20,
           }},
          {std::pair(kTTUnseal, RegisteredProof::StackedDRG2KiBSeal),
           Resources{
               .min_memory = uint64_t(2) << 10,
               .max_memory = uint64_t(2) << 10,
               .threads = 1,
               .can_gpu = false,
               .base_min_memory = uint64_t(2) << 10,
           }},
  };

}  // namespace fc::primitives

#endif  // CPP_FILECOIN_CORE_PRIMITIVES_RESOURCES_RESOURCES_HPP
