/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/randomness/impl/randomness_provider_impl.hpp"

#include <libp2p/crypto/sha/sha256.hpp>
#include "common/le_encoder.hpp"

namespace fc::crypto::randomness {

  int64_t kNoIndex = -1;

  Randomness RandomnessProviderImpl::deriveRandomness(DomainSeparationTag tag,
                                                      Serialization s) {
    return deriveRandomness(tag, std::move(s), kNoIndex);
  }

  Randomness RandomnessProviderImpl::deriveRandomness(DomainSeparationTag tag,
                                                      Serialization s,
                                                      const ChainEpoch &index) {
    return deriveRandomnessInternal(
        static_cast<uint64_t>(tag), std::move(s), index);
  }

  Randomness RandomnessProviderImpl::deriveRandomnessInternal(uint64_t tag,
                                                              Serialization s,
                                                              int64_t index) {
    common::Buffer value{};
    const size_t bytes_required =
        sizeof(DomainSeparationTag) + sizeof(ChainEpoch) + s.size();
    value.reserve(bytes_required);
    common::encodeLebInteger(tag, value);
    common::encodeLebInteger(index, value);
    value.put(s);
    auto hash = libp2p::crypto::sha256(value);

    return Randomness(hash);
  }

  int RandomnessProviderImpl::randomInt(const Randomness &randomness,
                                        size_t nonce,
                                        size_t limit) {
    // TODO(artyom-yurin): [FIL-141] Implement it
    throw std::string("Not implemented");
  }

}  // namespace fc::crypto::randomness
