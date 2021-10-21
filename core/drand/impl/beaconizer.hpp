/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include <boost/compute/detail/lru_cache.hpp>
#include <gsl/span>
#include <libp2p/basic/scheduler.hpp>

#include "drand/beaconizer.hpp"
#include "fwd.hpp"

namespace fc::drand {
  using boost::asio::io_context;
  using clock::UTCClock;
  using libp2p::basic::Scheduler;

  struct DrandScheduleImpl : DrandSchedule {
    DrandScheduleImpl(const ChainInfo &info,
                      seconds fc_genesis,
                      seconds fc_period)
        : drand_genesis{info.genesis},
          drand_period{info.period},
          fc_genesis{fc_genesis},
          fc_period{fc_period} {}

    Round maxRound(ChainEpoch epoch) const override;

    /** Drand genesis time */
    seconds drand_genesis;

    /** Drand round time */
    seconds drand_period;

    /** FFilecoin genesis time */
    seconds fc_genesis;

    /** Filecoin round time */
    seconds fc_period;
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

    boost::optional<Bytes> lookupCache(Round round);

    void cacheEntry(Round round, const Bytes &signature);

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
    boost::compute::detail::lru_cache<Round, Bytes> cache_;

    std::unique_ptr<crypto::bls::BlsProvider> bls_;
  };
}  // namespace fc::drand

OUTCOME_HPP_DECLARE_ERROR(fc::drand, BeaconizerImpl::Error);
