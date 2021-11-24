/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <prometheus/histogram.h>
#include <prometheus/registry.h>

namespace fc {
  inline auto &prometheusRegistry() {
    static prometheus::Registry x;
    return x;
  }

  constexpr std::initializer_list<double> kDefaultPrometheusMsBuckets{
      0.01, 0.05, 0.1,  0.3,  0.6,  0.8,  1,     2,     3,     4,      5,
      6,    8,    10,   13,   16,   20,   25,    30,    40,    50,     65,
      80,   100,  130,  160,  200,  250,  300,   400,   500,   650,    800,
      1000, 2000, 3000, 4000, 5000, 7500, 10000, 20000, 50000, 100000,
  };
}  // namespace fc
