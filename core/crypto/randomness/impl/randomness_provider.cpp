/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/randomness/impl/randomness_provider.hpp"

#include "common/le_encoder.hpp"
#include "crypto/sha/sha256.hpp"

namespace fc::crypto::randomness {

  Randomness RandomnessProviderImpl::deriveRandomness(DomainSeparationTag tag,
                                                      Serialization s) {
    return deriveRandomness(tag, std::move(s), static_cast<size_t>(-1));
  }

  Randomness RandomnessProviderImpl::deriveRandomness(DomainSeparationTag tag,
                                                      Serialization s,
                                                      ChainEpoch index) {
    return deriveRandomnessInternal(tag, std::move(s), index);
  }

  Randomness RandomnessProviderImpl::deriveRandomnessInternal(
      DomainSeparationTag tag, Serialization s, ChainEpoch index) {
    common::Buffer value{};
    const size_t bytes_required =
        sizeof(DomainSeparationTag) + sizeof(ChainEpoch) + s.size();
    value.reserve(bytes_required);
    common::encodeInteger(static_cast<size_t>(tag), value);
    common::encodeInteger(static_cast<size_t>(index), value);
    value.put(s);
    auto hash = sha256(value);

    return Randomness(hash);
  }

}  // namespace fc::crypto::randomness
