/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "core/test_vectors/fixed_randomness.hpp"

namespace fc::vm::runtime {

  enum class RandomnessType { kChain, kBeacon };

  /**
   * Values from test vectors
   */
  struct TestVectorRandomness {
    /** Ticket or beacon */
    RandomnessType type;

    /** Tag */
    DomainSeparationTag domain_separation_tag;

    /** Epoch */
    ChainEpoch epoch;

    /** Seed */
    Buffer entropy;

    /** Randomness result */
    Randomness ret;
  };

  /**
   * Randomness provider with predefined values
   *
   * Returns values defined in replaying_values or default fixed randomness
   */
  class ReplayingRandomness : public FixedRandomness {
   public:
    explicit ReplayingRandomness(
        std::vector<TestVectorRandomness> replaying_values);

    outcome::result<Randomness> getRandomnessFromTickets(
        const TsBranchPtr &ts_branch,
        DomainSeparationTag tag,
        ChainEpoch epoch,
        gsl::span<const uint8_t> seed) const override;

    outcome::result<Randomness> getRandomnessFromBeacon(
        const TsBranchPtr &ts_branch,
        DomainSeparationTag tag,
        ChainEpoch epoch,
        gsl::span<const uint8_t> seed) const override;

   private:
    /**
     * Returns predefined randomness if present or none otherwise
     */
    boost::optional<Randomness> getReplayingRandomness(
        RandomnessType type,
        DomainSeparationTag tag,
        ChainEpoch epoch,
        gsl::span<const uint8_t> seed) const;

    std::vector<TestVectorRandomness> test_vector_randomness_;
  };

}  // namespace fc::vm::runtime
