/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_PRIMITIVES_RLE_BITSET_RUNS_UTILS_HPP
#define CPP_FILECOIN_CORE_PRIMITIVES_RLE_BITSET_RUNS_UTILS_HPP

#include <gsl/span>
#include "common/outcome.hpp"
#include "primitives/bitvec/bitvec.hpp"

namespace fc::primitives {
  constexpr uint64_t kRunsVersion = 0;

  struct DecodeInfo {
    uint8_t length;
    uint8_t repeats;  // repeats + 1 is number of repeats of above run lengths
    uint8_t n;        // # of bits
    bool is_varint;
  };

  inline bool operator==(const DecodeInfo &lhs, const DecodeInfo &rhs) {
    return lhs.length == rhs.length && lhs.repeats == rhs.repeats
           && lhs.n == rhs.n && lhs.is_varint == rhs.is_varint;
  }

  const std::vector<DecodeInfo> kDecodeTable = {
      {0, 0, 2, true}, {1, 0, 1, false}, {0, 0, 6, false},  {1, 1, 2, false},

      {0, 0, 2, true}, {1, 0, 1, false}, {1, 0, 6, false},  {1, 2, 3, false},

      {0, 0, 2, true}, {1, 0, 1, false}, {2, 0, 6, false},  {1, 1, 2, false},

      {0, 0, 2, true}, {1, 0, 1, false}, {3, 0, 6, false},  {1, 3, 4, false},

      {0, 0, 2, true}, {1, 0, 1, false}, {4, 0, 6, false},  {1, 1, 2, false},

      {0, 0, 2, true}, {1, 0, 1, false}, {5, 0, 6, false},  {1, 2, 3, false},

      {0, 0, 2, true}, {1, 0, 1, false}, {6, 0, 6, false},  {1, 1, 2, false},

      {0, 0, 2, true}, {1, 0, 1, false}, {7, 0, 6, false},  {1, 4, 5, false},

      {0, 0, 2, true}, {1, 0, 1, false}, {8, 0, 6, false},  {1, 1, 2, false},

      {0, 0, 2, true}, {1, 0, 1, false}, {9, 0, 6, false},  {1, 2, 3, false},

      {0, 0, 2, true}, {1, 0, 1, false}, {10, 0, 6, false}, {1, 1, 2, false},

      {0, 0, 2, true}, {1, 0, 1, false}, {11, 0, 6, false}, {1, 3, 4, false},

      {0, 0, 2, true}, {1, 0, 1, false}, {12, 0, 6, false}, {1, 1, 2, false},

      {0, 0, 2, true}, {1, 0, 1, false}, {13, 0, 6, false}, {1, 2, 3, false},

      {0, 0, 2, true}, {1, 0, 1, false}, {14, 0, 6, false}, {1, 1, 2, false},

      {0, 0, 2, true}, {1, 0, 1, false}, {15, 0, 6, false}, {1, 5, 6, false},
  };

  outcome::result<std::vector<uint64_t>> runsFromBuffer(
      const std::vector<uint8_t> &buffer);

  std::vector<uint64_t> runsAnd(gsl::span<const uint64_t> lhs,
                                gsl::span<const uint64_t> rhs,
                                bool is_subtract = false);

  std::vector<uint64_t> runsOr(gsl::span<const uint64_t> lhs,
                               gsl::span<const uint64_t> rhs);

  outcome::result<uint64_t> runsCount(gsl::span<const uint64_t> runs);

  std::vector<uint64_t> runsFill(gsl::span<const uint64_t> runs);

  enum class RunsError {
    kRleOverflow = 1,
    kNotMinEncoded,
    kWrongVersion,
    kInvalidDecode,
    kLongRle,
  };

}  // namespace fc::primitives

OUTCOME_HPP_DECLARE_ERROR(fc::primitives, RunsError);

#endif  // CPP_FILECOIN_CORE_PRIMITIVES_RLE_BITSET_RUNS_UTILS_HPP
