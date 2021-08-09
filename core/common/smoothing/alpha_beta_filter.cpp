/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "common/smoothing/alpha_beta_filter.hpp"
#include "common/math/math.hpp"

namespace fc::common::smoothing {
  BigInt estimate(const FilterEstimate &filter) {
    return filter.position >> kPrecision128;
  }

  FilterEstimate nextEstimate(const FilterEstimate &previous_estimate,
                              const BigInt &observation,
                              uint64_t delta) {
    // to Q.128 format
    const BigInt delta_t = BigInt(delta) << kPrecision128;
    // Q.128 * Q.128 => Q.256
    BigInt delta_x = delta_t * previous_estimate.velocity;
    // Q.256 => Q.128
    delta_x >>= kPrecision128;
    BigInt position = previous_estimate.position + delta_x;

    // observation Q.0 => Q.128
    const BigInt residual = (observation << kPrecision128) - position;
    // Q.128 * Q.128 => Q.256
    BigInt revision_x = kDefaultAlpha * residual;
    // Q.256 => Q.128
    revision_x >>= kPrecision128;
    position += revision_x;

    // Q.128 * Q.128 => Q.256
    BigInt revision_v = kDefaultBeta * residual;
    // Q.256 / Q.128 => Q.128
    revision_v = bigdiv(revision_v, delta_t);
    const BigInt velocity = previous_estimate.velocity + revision_v;

    return {position, velocity};
  }

  BigInt extrapolatedCumSumOfRatio(uint64_t delta,
                                   uint64_t start,
                                   const FilterEstimate &num,
                                   const FilterEstimate &den) {
    BigInt delta_t{BigInt{delta} << kPrecision128};  // Q.0 => Q.128
    BigInt t0{BigInt{start} << kPrecision128};       // Q.0 => Q.128
    const auto &position1{num.position};
    const auto &position2{den.position};
    const auto &velocity1{num.velocity};
    const auto &velocity2{den.velocity};

    BigInt squared_velocity2{velocity2 * velocity2};  // Q.128 * Q.128 => Q.256
    squared_velocity2 >>= kPrecision128;              // Q.256 => Q.128

    static const BigInt kEpsilon{"302231454903657293676544"};
    if (squared_velocity2 > kEpsilon) {
      BigInt x2a{t0 * velocity2};  // Q.128 * Q.128 => Q.256
      x2a >>= kPrecision128;       // Q.256 => Q.128
      x2a += position2;

      BigInt x2b{delta_t * velocity2};  // Q.128 * Q.128 => Q.256
      x2b >>= kPrecision128;            // Q.256 => Q.128
      x2b += x2a;

      x2a = common::math::ln(x2a);  // Q.128
      x2b = common::math::ln(x2b);  // Q.128

      BigInt m1{x2b - x2a};
      m1 *= velocity2 * position1;  // Q.128 * Q.128 * Q.128 => Q.384
      m1 >>= kPrecision128;         // Q.384 => Q.256

      BigInt m2L{x2a - x2b};
      m2L *= position2;                 // Q.128 * Q.128 => Q.256
      BigInt m2R{velocity2 * delta_t};  // Q.128 * Q.128 => Q.256
      BigInt m2{m2L + m2R};
      m2 *= velocity1;       // Q.256 => Q.384
      m2 >>= kPrecision128;  // Q.384 => Q.256

      return bigdiv(m1 + m2, squared_velocity2);  // Q.256 / Q.128 => Q.128
    }

    BigInt half_delta_t{delta_t >> 1};            // Q.128 / Q.0 => Q.128
    BigInt x1m{velocity1 * (t0 + half_delta_t)};  // Q.128 * Q.128 => Q.256
    x1m >>= kPrecision128;                        // Q.256 => Q.128
    x1m += position1;

    BigInt cumsumRatio{x1m * delta_t};  // Q.128 * Q.128 => Q.256
    cumsumRatio /= position2;           // Q.256 / Q.128 => Q.128
    return cumsumRatio;
  }
}  // namespace fc::common::smoothing
