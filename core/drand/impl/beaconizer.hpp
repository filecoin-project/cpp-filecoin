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

#include "drand/beaconizer.hpp"
#include "node/fwd.hpp"

namespace fc::drand {
  using boost::asio::io_context;
  using clock::UTCClock;
  using libp2p::protocol::Scheduler;

  struct DrandScheduleImpl : DrandSchedule {
    DrandScheduleImpl(const ChainInfo &info,
                      seconds fc_genesis,
                      seconds fc_period)
        : genesis{info.genesis},
          period{info.period},
          fc_genesis{fc_genesis},
          fc_period{fc_period} {}

    Round maxRound(ChainEpoch epoch) const override {
      return ((epoch - 1) * fc_period + fc_genesis - genesis) / period;
    }

    seconds genesis, period, fc_genesis, fc_period;
  };

  class BeaconizerImpl : public Beaconizer,
                         public std::enable_shared_from_this<BeaconizerImpl> {
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

    BeaconizerImpl(std::shared_ptr<io_context> io,
                   std::shared_ptr<UTCClock> clock,
                   std::shared_ptr<Scheduler> scheduler,
                   const ChainInfo &info,
                   std::vector<std::string> drand_servers,
                   size_t max_cache_size);

    void entry(Round round, CbT<BeaconEntry> cb) override;

    outcome::result<void> verifyEntry(const BeaconEntry &current,
                                      const BeaconEntry &previous) override;

   private:
    //
    // METHODS
    //

    boost::optional<Buffer> lookupCache(Round round);

    void cacheEntry(Round round, const Buffer &signature);

    outcome::result<bool> verifyBeaconData(
        uint64_t round,
        gsl::span<const uint8_t> signature,
        gsl::span<const uint8_t> previous_signature);

    void rotatePeersIndex();

    //
    // FIELDS
    //

    std::shared_ptr<io_context> io;
    std::shared_ptr<UTCClock> clock;
    std::shared_ptr<Scheduler> scheduler;

    ChainInfo info;

    std::atomic_size_t peer_index_;
    std::vector<std::string> peers_;

    std::mutex cache_mutex_;
    boost::compute::detail::lru_cache<Round, Buffer> cache_;

    std::unique_ptr<crypto::bls::BlsProvider> bls_;
  };
}  // namespace fc::drand

OUTCOME_HPP_DECLARE_ERROR(fc::drand, BeaconizerImpl::Error);

#endif  // CPP_FILECOIN_CORE_DRAND_IMPL_BEACONIZER_HPP
