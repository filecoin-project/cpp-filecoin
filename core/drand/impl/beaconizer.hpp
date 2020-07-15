/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_DRAND_IMPL_BEACONIZER_HPP
#define CPP_FILECOIN_CORE_DRAND_IMPL_BEACONIZER_HPP

#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include <boost/compute/detail/lru_cache.hpp>
#include <gsl/span>

#include "crypto/bls/bls_types.hpp"
#include "drand/beaconizer.hpp"
#include "drand/client.hpp"

namespace fc::crypto::bls {
  class BlsProvider;
}

namespace fc::drand {

  class BeaconizerImpl : public Beaconizer {
   public:
    enum class Error {
      kNoPublicKey = 1,
      kNetworkKeyMismatch,
      kEmptyServersList,
      kZeroCacheSize,
      kInvalidSignatureFormat,
      kInvalidBeacon,
      kNegativeEpoch,
    };

    /**
     * Creates and initializes Drand beacons manager
     * @param filecoin_genesis_time - filecoin chain genesis time
     * @param filecoin_round_time - filecoin rounds interval
     * @param drand_servers - list of drand servers' addresses with port
     * specified
     * @param network_public_key - known key for the drand network
     * @param max_cache_size - beacon entries cache limit
     * @return unique pointer to an instance
     */
    static outcome::result<std::unique_ptr<BeaconizerImpl>> create(
        uint64_t filecoin_genesis_time,
        uint64_t filecoin_round_time,
        std::vector<std::string> drand_servers,
        gsl::span<const uint8_t> network_public_key,
        size_t max_cache_size);

    outcome::result<BeaconEntry> entry(uint64_t round) override;

    outcome::result<void> verifyEntry(const BeaconEntry &current,
                                      const BeaconEntry &previous) override;

    outcome::result<uint64_t> maxBeaconRoundForEpoch(
        ChainEpoch fil_epoch) override;

   private:
    //
    // METHODS
    //

    BeaconizerImpl(uint64_t filecoin_genesis_time,
                   uint64_t filecoin_round_time,
                   std::vector<std::string> drand_servers,
                   crypto::bls::PublicKey network_public_key,
                   size_t max_cache_size);

    outcome::result<void> init();

    outcome::result<void> verifyNetworkKey(const Bytes &key) const;

    boost::optional<Bytes> lookupCache(uint64_t round);

    void cacheEntry(uint64_t round, const Bytes &signature);

    outcome::result<bool> verifyBeaconData(
        uint64_t round,
        gsl::span<const uint8_t> signature,
        gsl::span<const uint8_t> previous_signature);

    void rotatePeersIndex();

    // creates a client to the currently chosen peer
    void dial();

    //
    // FIELDS
    //

    uint64_t fil_gen_time_;
    uint64_t fil_round_time_;

    std::atomic_size_t peer_index_;
    std::vector<std::string> peers_;

    const crypto::bls::PublicKey network_key_;

    std::mutex cache_mutex_;
    boost::compute::detail::lru_cache<uint64_t, Bytes> cache_;

    std::unique_ptr<crypto::bls::BlsProvider> bls_;
    std::unique_ptr<DrandSyncClient> client_;

    uint64_t drand_gen_time_;
    uint64_t drand_interval_;
  };
}  // namespace fc::drand

OUTCOME_HPP_DECLARE_ERROR(fc::drand, BeaconizerImpl::Error);

#endif  // CPP_FILECOIN_CORE_DRAND_IMPL_BEACONIZER_HPP
