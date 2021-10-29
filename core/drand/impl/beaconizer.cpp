/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "drand/impl/beaconizer.hpp"

#include <boost/random.hpp>
#include <libp2p/common/byteutil.hpp>
#include "crypto/sha/sha256.hpp"

#include "clock/utc_clock.hpp"
#include "common/logger.hpp"
#include "common/outcome_fmt.hpp"
#include "crypto/bls/impl/bls_provider_impl.hpp"
#include "drand/impl/http.hpp"

#define MOVE(x)  \
  x {            \
    std::move(x) \
  }

OUTCOME_CPP_DEFINE_CATEGORY(fc::drand, BeaconizerImpl::Error, e) {
  using E = fc::drand::BeaconizerImpl::Error;
  switch (e) {
    case E::kNoPublicKey:
      return "Drand node did not return dist key.";
    case E::kNetworkKeyMismatch:
      return "Drand peer's key don't match the known network key.";
    case E::kEmptyServersList:
      return "No drand servers specified to connect to.";
    case E::kZeroCacheSize:
      return "Cache size cannot be zero.";
    case E::kInvalidSignatureFormat:
      return "Signature has invalid format.";
    case E::kInvalidBeacon:
      return "Beacon did not pass the verification";
    case E::kNegativeEpoch:
      return "Negative filecoin epoch is not allowed in calculations";
  }
}

namespace fc::drand {
  Round DrandScheduleImpl::maxRound(ChainEpoch epoch) const {
    BOOST_ASSERT_MSG(drand_period.count() > 0, "drand period must be > 0");
    return ((epoch - 1) * fc_period + fc_genesis - drand_genesis)
           / drand_period;
  }

  BeaconizerImpl::BeaconizerImpl(std::shared_ptr<io_context> io,
                                 std::shared_ptr<UTCClock> clock,
                                 std::shared_ptr<Scheduler> scheduler,
                                 const ChainInfo &info,
                                 std::vector<std::string> drand_servers,
                                 size_t max_cache_size)
      : MOVE(io),
        MOVE(clock),
        MOVE(scheduler),
        info{info},
        peers_{std::move(drand_servers)},
        cache_{max_cache_size},
        bls_{std::make_unique<crypto::bls::BlsProviderImpl>()} {
    assert(!peers_.empty());
    assert(max_cache_size != 0);
    rotatePeersIndex();
  }

  void BeaconizerImpl::entry(Round round, CbT<BeaconEntry> cb) {
    if (auto cached{lookupCache(round)}) {
      return cb(BeaconEntry{round, std::move(*cached)});
    }
    auto fetch{[self{shared_from_this()}, round, MOVE(cb)]() {
      http::getEntry(*self->io,
                     self->peers_[self->peer_index_],
                     round,
                     [self, MOVE(cb), round](auto &&_res) {
                       std::error_code error;
                       if (!_res) {
                         error = _res.error();
                       } else {
                         auto res{_res.value()};
                         if (res.round != round) {
                           error = Error::kInvalidBeacon;
                         } else {
                           BeaconEntry entry{round, copy(res.signature)};
                           auto _valid{self->verifyEntry(
                               entry, {round - 1, std::move(res.prev)})};
                           if (!_valid) {
                             error = _valid.error();
                           } else {
                             return cb(std::move(entry));
                           }
                         }
                       }
                       spdlog::error("drand host {} error: {:#}",
                                     self->peers_[self->peer_index_],
                                     error);
                       self->rotatePeersIndex();
                       // TODO(turuslan): retry
                       return cb(error);
                     });
    }};
    auto now{clock->nowUTC()};
    auto time{info.genesis + round * info.period};
    if (now < time) {
      scheduler->schedule(std::move(fetch), time - now);
    } else {
      fetch();
    }
  }

  outcome::result<void> BeaconizerImpl::verifyEntry(
      const BeaconEntry &current, const BeaconEntry &previous) {
    if (0 == previous.round || lookupCache(current.round)) {
      return outcome::success();
    }
    OUTCOME_TRY(is_valid,
                verifyBeaconData(current.round, current.data, previous.data));
    if (not is_valid) {
      return Error::kInvalidBeacon;
    }
    cacheEntry(current.round, current.data);
    return outcome::success();
  }

  // private stuff goes below

  boost::optional<Bytes> BeaconizerImpl::lookupCache(Round round) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    auto bytes = cache_.get(round);
    return bytes;
  }

  void BeaconizerImpl::cacheEntry(Round round, const Bytes &signature) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    cache_.insert(round, signature);
  }

  outcome::result<bool> BeaconizerImpl::verifyBeaconData(
      uint64_t round,
      gsl::span<const uint8_t> signature,
      gsl::span<const uint8_t> previous_signature) {
    auto message = [round, &previous_signature]() {
      std::vector<uint8_t> buffer;
      buffer.reserve(previous_signature.size() + sizeof(uint64_t));
      buffer.insert(
          buffer.end(), previous_signature.begin(), previous_signature.end());
      libp2p::common::putUint64BE(buffer, round);
      auto hash = crypto::sha::sha256(buffer);
      return hash;
    };
    crypto::bls::Signature bls_sig;
    if (bls_sig.size() != signature.size()) {
      return Error::kInvalidSignatureFormat;
    }
    std::copy(signature.begin(), signature.end(), bls_sig.begin());
    OUTCOME_TRY(is_valid, bls_->verifySignature(message(), bls_sig, info.key));
    return is_valid;
  }

  void BeaconizerImpl::rotatePeersIndex() {
    boost::random::mt19937 rng;
    boost::random::uniform_int_distribution<> generator(
        0, gsl::narrow<int>(peers_.size()) - 1);
    auto new_index = generator(rng);
    peer_index_.store(new_index);
  }
}  // namespace fc::drand
