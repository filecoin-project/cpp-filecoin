/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "core/test_vectors/replaying_randomness.hpp"

namespace fc::vm::runtime {

  ReplayingRandomness::ReplayingRandomness(
      std::vector<TestVectorRandomness> replaying_values)
      : test_vector_randomness_{std::move(replaying_values)} {}

  outcome::result<Randomness> ReplayingRandomness::getRandomnessFromTickets(
      const TipsetCPtr &,
      DomainSeparationTag tag,
      ChainEpoch epoch,
      gsl::span<const uint8_t> seed) const {
    auto maybe_randomness =
        getReplayingRandomness(RandomnessType::kChain, tag, epoch, seed);
    if (maybe_randomness) {
      return maybe_randomness.value();
    }
    return FixedRandomness::getRandomnessFromTickets(
        TipsetCPtr{}, tag, epoch, seed);
  }

  outcome::result<Randomness> ReplayingRandomness::getRandomnessFromBeacon(
      const TipsetCPtr &,
      DomainSeparationTag tag,
      ChainEpoch epoch,
      gsl::span<const uint8_t> seed) const {
    auto maybe_randomness =
        getReplayingRandomness(RandomnessType::kBeacon, tag, epoch, seed);
    if (maybe_randomness) {
      return maybe_randomness.value();
    }
    return FixedRandomness::getRandomnessFromBeacon(
        TipsetCPtr{}, tag, epoch, seed);
  }

  boost::optional<Randomness> ReplayingRandomness::getReplayingRandomness(
      RandomnessType type,
      DomainSeparationTag tag,
      ChainEpoch epoch,
      gsl::span<const uint8_t> seed) const {
    for (const auto &rand : test_vector_randomness_) {
      if (rand.type == type && rand.domain_separation_tag == tag
          && rand.epoch == epoch
          && std::equal(rand.entropy.begin(),
                        rand.entropy.end(),
                        seed.cbegin(),
                        seed.cend())) {
        return rand.ret;
      }
    }
    return boost::none;
  }

}  // namespace fc::vm::runtime
