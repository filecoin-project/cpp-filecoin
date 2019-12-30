/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "crypto/vrf/impl/vrf_provider_impl.hpp"

#include <libp2p/multi/multihash.hpp>

#include "common/le_encoder.hpp"
#include "common/outcome.hpp"
#include "primitives/address/address_codec.hpp"

namespace fc::crypto::vrf {

  VRFProviderImpl::VRFProviderImpl(
      std::shared_ptr<bls::BlsProvider> bls_provider)
      : bls_provider_(std::move(bls_provider)) {}

  outcome::result<VRFResult> VRFProviderImpl::generateVRF(
      randomness::DomainSeparationTag tag,
      const VRFSecretKey &worker_priv_key,
      const Buffer &miner_bytes,
      const Buffer &message) const {
    OUTCOME_TRY(hash_base, vrfHash(tag, miner_bytes, message));
    return bls_provider_->sign(message, worker_priv_key);
  }

  outcome::result<bool> VRFProviderImpl::verifyVRF(
      randomness::DomainSeparationTag tag,
      const VRFSecretKey &worker_priv_key,
      const Buffer &miner_bytes,
      const Buffer &message,
      const VRFProof &vrf_proof) const {
    OUTCOME_TRY(hash_base, vrfHash(tag, miner_bytes, message));
    return bls_provider_->verifySignature(message, vrf_proof, )
  }

  outcome::result<common::Hash256> VRFProviderImpl::vrfHash(
      randomness::DomainSeparationTag tag,
      const Buffer &miner_bytes,
      const Buffer &message) const {
    Buffer out{};
    auto required_bytes = sizeof(uint64_t) + 2 * sizeof(uint8_t)
                          + message.size() + miner_bytes.size();
    out.reserve(required_bytes);
    common::encodeInteger(static_cast<uint64_t>(tag), out);
    out.putUint8(0u);
    out.putBuffer(message);
    out.putUint8(0u);
    out.put(miner_bytes);
    OUTCOME_TRY(multi_hash, libp2p::multi::Multihash::create(libp2p::multi::HashType::sha256, out));

    return common::Hash256::fromSpan(multi_hash.toBuffer());
  }
}  // namespace fc::crypto::vrf

OUTCOME_CPP_DEFINE_CATEGORY(fc::crypto::vrf, VRFError, e) {
  using fc::crypto::vrf::VRFError;
  switch (e) {
    case (VRFError::MINER_ADDRESS_NOT_ID):
      return "miner address has to be of ID type to calculate hash";
  }

  return "unknown error";
}
