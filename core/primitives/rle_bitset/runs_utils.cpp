/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/rle_bitset/runs_utils.hpp"

namespace {
  using fc::primitives::RunsError;
  using fc::primitives::bitvec::BitvecReader;

  fc::outcome::result<uint64_t> decodeVarint(BitvecReader &reader) {
    uint64_t result = 0;
    uint64_t size = 0;

    for (int i = 0; i < 10; ++i) {
      uint8_t byte = reader.getByte();
      if (byte < 0x80) {
        if (i == 9 && byte > 1) {
          break;
        }
        if (byte == 0 && size > 0) {
          return RunsError::kInvalidDecode;
        }
        result |= uint64_t(byte) << size;
        return result;
      }
      result |= uint64_t(byte & 0x7f) << size;
      size += 7;
    }
    return RunsError::kLongRle;
  }
}  // namespace

namespace fc::primitives {

  outcome::result<std::vector<uint64_t>> runsFromBuffer(
      const std::vector<uint8_t> &buffer) {
    if (not buffer.empty() && buffer[buffer.size() - 1] == 0) {
      return RunsError::kNotMinEncoded;
    }

    BitvecReader reader(buffer);
    auto version = reader.get(2);
    if (version != kRunsVersion) {
      return RunsError::kWrongVersion;
    }

    bool is_first_offset = reader.get(1) != 1;
    uint8_t i = 0;
    uint64_t length = 0;

    auto getDecodeData = [&reader, &i, &length]() -> outcome::result<void> {
      auto idx = reader.peek6Bit();
      auto decode = kDecodeTable[idx];
      reader.get(decode.n);

      i = decode.repeats;
      length = decode.length;
      if (decode.is_varint) {
        OUTCOME_TRYA(length, decodeVarint(reader));
      }

      return outcome::success();
    };

    OUTCOME_TRY(getDecodeData());

    std::vector<uint64_t> result = {};

    if (!is_first_offset) {
      result.push_back(0);
    }

    while (length != 0) {
      result.push_back(length);

      if (i == 0) {
        OUTCOME_TRY(getDecodeData());
      } else {
        --i;
      }
    }

    return result;
  }

  // TODO (a.chernyshov) The function is too complex, should be simplified
  // NOLINTNEXTLINE(readability-function-cognitive-complexity)
  std::vector<uint64_t> runsAnd(gsl::span<const uint64_t> lhs,
                                gsl::span<const uint64_t> rhs,
                                bool is_subtract) {
    auto lhs_itr = lhs.begin();
    auto rhs_itr = rhs.cbegin();
    bool is_value_lhs = true;
    bool is_value_rhs = !is_subtract;
    uint64_t lhs_value = 0;
    uint64_t rhs_value = 0;

    auto nextRun = [&]() -> std::pair<uint64_t, bool> {
      bool is_value_next = false;
      uint64_t next_value = 0;

      while (true) {
        if (lhs_value == 0) {
          if (lhs_itr == lhs.cend()) {
            break;
          }
          lhs_value = *(lhs_itr++);
          is_value_lhs = !is_value_lhs;
        }

        if (rhs_value == 0) {
          if (rhs_itr == rhs.cend()) {
            if (not is_subtract) break;
            rhs_value = std::numeric_limits<uint64_t>::max();
            is_value_rhs = true;
          } else {
            rhs_value = *(rhs_itr++);
            is_value_rhs = !is_value_rhs;
          }
        }

        bool is_value_new = is_value_lhs && is_value_rhs;

        if (next_value > 0 && is_value_next != is_value_new) {
          return {next_value, is_value_next};
        }

        uint64_t new_value = std::min(lhs_value, rhs_value);

        is_value_next = is_value_new;
        next_value += new_value;
        lhs_value -= new_value;
        rhs_value -= new_value;
      }

      if (next_value != 0) {
        return {next_value, is_value_next};
      }

      return {0, false};
    };

    std::vector<uint64_t> result = {0};
    bool should_be = true;

    auto new_run = nextRun();
    while (new_run.first != 0) {
      if (new_run.second == should_be) {
        result.push_back(new_run.first);
        should_be = !should_be;
      } else {
        result.back() += new_run.first;
      }
      new_run = nextRun();
    }

    return result;
  }

  // TODO (a.chernyshov) The function is too complex, should be simplified
  // NOLINTNEXTLINE(readability-function-cognitive-complexity)
  std::vector<uint64_t> runsOr(gsl::span<const uint64_t> lhs,
                               gsl::span<const uint64_t> rhs) {
    auto lhs_itr = lhs.begin();
    auto rhs_itr = rhs.cbegin();

    bool is_value_lhs = true;
    uint64_t lhs_value = 0;

    bool is_value_rhs = true;
    uint64_t rhs_value = 0;

    if (lhs_itr != lhs.end() && *lhs_itr == 0) {
      is_value_lhs = false;
      ++lhs_itr;
    }

    if (rhs_itr != rhs.end() && *rhs_itr == 0) {
      is_value_rhs = false;
      ++rhs_itr;
    }

    auto nextRun = [&]() -> std::pair<uint64_t, bool> {
      bool is_value_next = false;
      uint64_t next_value = 0;

      auto fetch = [&]() {
        if (lhs_value == 0 && lhs_itr != lhs.end()) {
          lhs_value = *lhs_itr;
          is_value_lhs = !is_value_lhs;
          lhs_itr++;
        }
        if (rhs_value == 0 && rhs_itr != rhs.end()) {
          rhs_value = *rhs_itr;
          is_value_rhs = !is_value_rhs;
          rhs_itr++;
        }
      };

      fetch();

      if (lhs_value == 0) {
        is_value_next = is_value_rhs;
        next_value = rhs_value;
        rhs_value = 0;
        return {next_value, is_value_next};
      }

      if (rhs_value == 0) {
        is_value_next = is_value_lhs;
        next_value = lhs_value;
        lhs_value = 0;
        return {next_value, is_value_next};
      }

      if (!is_value_lhs && !is_value_rhs) {
        next_value = std::min(lhs_value, rhs_value);
        is_value_next = is_value_lhs;
        lhs_value -= next_value;
        rhs_value -= next_value;

        fetch();

        auto trailing_run = [&](std::pair<uint64_t, bool> lhs,
                                std::pair<uint64_t, bool> rhs) {
          return lhs.first == 0 && rhs.second == is_value_next;
        };
        if (trailing_run(std::make_pair(lhs_value, is_value_lhs),
                         std::make_pair(rhs_value, is_value_rhs))) {
          next_value += lhs_value;
          next_value += rhs_value;
          lhs_value = 0;
          rhs_value = 0;
        }
        return {next_value, is_value_next};
      }

      is_value_next = true;
      while ((is_value_lhs && (lhs_value != 0))
             || (is_value_rhs && (rhs_value != 0))) {
        auto min = lhs_value;
        if ((rhs_value < min && rhs_value != 0) || lhs_value == 0) {
          min = rhs_value;
        }
        next_value += min;
        if (lhs_value != 0) {
          lhs_value -= min;
        }

        if (rhs_value != 0) {
          rhs_value -= min;
        }

        fetch();
      }

      return {next_value, is_value_next};
    };

    std::vector<uint64_t> result = {0};
    bool should_be = true;

    auto a = nextRun();
    while (a.first != 0) {
      if (a.second == should_be) {
        result.push_back(a.first);
        should_be = !should_be;
      } else {
        result.back() += a.first;
      }
      a = nextRun();
    }

    return result;
  }

  outcome::result<uint64_t> runsCount(gsl::span<const uint64_t> runs) {
    uint64_t length = 0;
    uint64_t count = 0;
    bool is_value = false;

    for (const auto &run : runs) {
      if (std::numeric_limits<uint64_t>::max() - run < length) {
        return RunsError::kRleOverflow;
      }
      length += run;

      if (is_value) {
        count += run;
      }

      is_value = !is_value;
    }

    return count;
  }

  std::vector<uint64_t> runsFill(gsl::span<const uint64_t> runs) {
    uint64_t index = 0;
    uint64_t length = 0;
    bool is_value = false;

    for (const auto &run : runs) {
      index += run;

      if (is_value) {
        length = index;
      }
      is_value = !is_value;
    }

    std::vector<uint64_t> result = {0};
    if (length > 0) {
      result.push_back(length);
    }

    return result;
  }
}  // namespace fc::primitives

OUTCOME_CPP_DEFINE_CATEGORY(fc::primitives, RunsError, e) {
  using fc::primitives::RunsError;
  switch (e) {
    case (RunsError::kRleOverflow):
      return "RunsUtil: RLE overflows";
    case (RunsError::kInvalidDecode):
      return "RunsUtil: invalid encoding for RLE";
    case (RunsError::kLongRle):
      return "RunsUtil: run too long";
    case (RunsError::kNotMinEncoded):
      return "RunsUtil: not minimally encoded";
    case (RunsError::kWrongVersion):
      return "RunsUtil: invalid RLE version";
    default:
      return "RunsUtil: unknown error";
  }
}
