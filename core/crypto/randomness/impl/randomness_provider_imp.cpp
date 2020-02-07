/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/randomness/impl/randomness_provider_imp.hpp"

#include <libp2p/crypto/sha/sha256.hpp>
#include "common/le_encoder.hpp"

namespace fc::crypto::randomness {

  Randomness RandomnessProviderImpl::deriveRandomness(DomainSeparationTag tag,
                                                      Serialization s) {
    return deriveRandomness(tag, std::move(s), -1);
  }

  Randomness RandomnessProviderImpl::deriveRandomness(DomainSeparationTag tag,
                                                      Serialization s,
                                                      const ChainEpoch &index) {
    return deriveRandomnessInternal(
        tag, std::move(s), index.convert_to<int64_t>());
  }

  Randomness RandomnessProviderImpl::deriveRandomnessInternal(
      DomainSeparationTag tag, Serialization s, int64_t index) {
    common::Buffer value{};
    const size_t bytes_required =
        sizeof(DomainSeparationTag) + sizeof(ChainEpoch) + s.size();
    value.reserve(bytes_required);
    common::encodeInteger(static_cast<size_t>(tag), value);
    common::encodeInteger(index, value);
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
