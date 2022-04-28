/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <spdlog/fmt/fmt.h>
#include <indicators/progress_bar.hpp>

namespace fc::storage::cids_index {
  /** detects value changes by step */
  struct Each {
    size_t step{};
    size_t value{};
    size_t next{};

    /** returns true if value changed by step */
    inline bool update() {
      if (step != 0 && value >= next) {
        next = step == 1 ? value + 1 : value - value % step + step;
        return true;
      }
      return false;
    }
  };

  inline std::string units(size_t value, const std::string_view &suffix) {
    return std::to_string(value).append(suffix);
  }
  /** formats value as max unit */
  template <typename... Ts>
  inline std::string units(size_t value,
                           size_t unit,
                           const std::string_view &suffix,
                           const Ts &...ts) {
    return value < unit ? units(value, ts...) : units(value / unit, suffix);
  }
  /** formats byte size */
  inline std::string bytesUnits(size_t value) {
    return units(value, 1 << 30, "gb", 1 << 20, "mb", 1 << 10, "kb", "b");
  }

  struct Progress {
    std::ofstream _null;
    indicators::ProgressBar bar{
        indicators::option::ShowPercentage{true},
        indicators::option::ShowElapsedTime{true},
        indicators::option::Completed{!isTty()},
        indicators::option::Stream{_null},
    };
    Each car_offset;
    Each items;
    size_t car_size{};
    size_t max_progress{};

    static inline bool isTty() {
      static auto cached{indicators::terminal_width() != 0};
      return cached;
    }

    inline void begin() {
      max_progress = car_size + 1;
      bar.set_option(indicators::option::MaxProgress{max_progress});
      bar.set_option(indicators::option::PrefixText{"reading "});
      bar.print_progress();
    }
    inline void update(bool force = false) {
      if (force || car_offset.update() || items.update()) {
        bar.set_option(indicators::option::PostfixText{
            fmt::format("{}/{}, {} items",
                        bytesUnits(car_offset.value),
                        bytesUnits(car_size),
                        items.value)});
        bar.set_progress(car_offset.value);
      }
    }
    inline void sort() {
      update(true);
      bar.set_option(indicators::option::PrefixText{"sorting "});
      bar.print_progress();
    }
    inline void end() {
      bar.set_option(indicators::option::PrefixText{"indexed "});
      bar.set_progress(max_progress);
    }
  };
}  // namespace fc::storage::cids_index
