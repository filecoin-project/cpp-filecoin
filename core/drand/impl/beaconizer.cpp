/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "drand/impl/beaconizer.hpp"

#include <boost/random.hpp>
#include <libp2p/common/byteutil.hpp>
#include <libp2p/crypto/sha/sha256.hpp>
#include "crypto/bls/impl/bls_provider_impl.hpp"

#include "drand/impl/client.hpp"

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
  outcome::result<std::unique_ptr<BeaconizerImpl>> BeaconizerImpl::create(
      uint64_t filecoin_genesis_time,
      uint64_t filecoin_round_time,
      std::vector<std::string> drand_servers,
      gsl::span<const uint8_t> network_public_key,
      size_t max_cache_size) {
    if (drand_servers.empty()) {
      return Error::kEmptyServersList;
    }
    crypto::bls::PublicKey net_key;
    if (network_public_key.size() != net_key.size()) {
      return crypto::bls::Errors::kInvalidPublicKey;
    }
    std::copy(
        network_public_key.begin(), network_public_key.end(), net_key.begin());
    if (0 == max_cache_size) {
      return Error::kZeroCacheSize;
    }
    struct make_unique_enabler : public BeaconizerImpl {
      make_unique_enabler(uint64_t filecoin_genesis_time,
                          uint64_t filecoin_round_time,
                          std::vector<std::string> drand_servers,
                          crypto::bls::PublicKey network_public_key,
                          size_t max_cache_size)
          : BeaconizerImpl{filecoin_genesis_time,
                           filecoin_round_time,
                           std::move(drand_servers),
                           std::move(network_public_key),
                           max_cache_size} {};
    };
    auto instance =
        std::make_unique<make_unique_enabler>(filecoin_genesis_time,
                                              filecoin_round_time,
                                              std::move(drand_servers),
                                              std::move(net_key),
                                              max_cache_size);
    OUTCOME_TRY(instance->init());
    return outcome::success(std::move(instance));
  }

  BeaconizerImpl::BeaconizerImpl(uint64_t filecoin_genesis_time,
                                 uint64_t filecoin_round_time,
                                 std::vector<std::string> drand_servers,
                                 crypto::bls::PublicKey network_public_key,
                                 size_t max_cache_size)
      : fil_gen_time_{filecoin_genesis_time},
        fil_round_time_{filecoin_round_time},
        peers_{std::move(drand_servers)},
        network_key_{std::move(network_public_key)},
        cache_{max_cache_size},
        bls_{std::make_unique<crypto::bls::BlsProviderImpl>()} {
    BOOST_ASSERT(not peers_.empty());
    BOOST_ASSERT(max_cache_size > 0);
  }

  outcome::result<BeaconEntry> BeaconizerImpl::entry(uint64_t round) {
    BeaconEntry entry{.round = round};
    auto bytes = lookupCache(round);
    if (bytes) {
      entry.data = std::move(bytes.value());
      return entry;
    }

    auto response = client_->publicRand(round);
    if (response) {
      entry.data = std::move(response.value().signature);
      return entry;
    }

    OUTCOME_TRY(init());
    // this place looks weird in lotus too :)
    return response.error();
  }

  outcome::result<void> BeaconizerImpl::verifyEntry(
      const BeaconEntry &current, const BeaconEntry &previous) {
    if (0 == previous.round) {
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

  outcome::result<uint64_t> BeaconizerImpl::maxBeaconRoundForEpoch(
      ChainEpoch fil_epoch) {
    if (fil_epoch < 0) {
      return Error::kNegativeEpoch;
    }
    auto epoch = static_cast<uint64_t>(fil_epoch);

    // sometimes the genesis time for filecoin is zero and this goes negative
    auto latest_ts =
        (epoch * fil_round_time_ + fil_gen_time_) - fil_round_time_;
    auto dround = (latest_ts - drand_gen_time_) / drand_interval_;
    return dround;
  }

  // private stuff goes below

  outcome::result<void> BeaconizerImpl::init() {
    rotatePeersIndex();
    dial();
    OUTCOME_TRY(group, client_->group());
    if (group.dist_key.empty()) {
      return Error::kNoPublicKey;
    }
    OUTCOME_TRY(verifyNetworkKey(group.dist_key[0]));
    drand_interval_ = group.period;
    drand_gen_time_ = group.genesis_time;
    return outcome::success();
  }

  outcome::result<void> BeaconizerImpl::verifyNetworkKey(
      const Bytes &key) const {
    auto &key1 = network_key_;
    if (key.size() != key1.size()) {
      return crypto::bls::Errors::kInvalidPublicKey;
    }
    if (not std::equal(key1.begin(), key1.end(), key.begin())) {
      return Error::kNetworkKeyMismatch;
    }
    return outcome::success();
  }

  boost::optional<Bytes> BeaconizerImpl::lookupCache(uint64_t round) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    auto bytes = cache_.get(round);
    return bytes;
  }

  void BeaconizerImpl::cacheEntry(uint64_t round, const Bytes &signature) {
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
      auto hash = libp2p::crypto::sha256(buffer);
      return hash;
    };
    crypto::bls::Signature bls_sig;
    if (bls_sig.size() != signature.size()) {
      return Error::kInvalidSignatureFormat;
    }
    std::copy(signature.begin(), signature.end(), bls_sig.begin());
    OUTCOME_TRY(is_valid,
                bls_->verifySignature(message(), bls_sig, network_key_));
    return is_valid;
  }

  void BeaconizerImpl::rotatePeersIndex() {
    boost::random::mt19937 rng;
    boost::random::uniform_int_distribution<> generator(1, peers_.size());
    auto new_index = generator(rng);
    peer_index_.store(new_index);
  }

  void BeaconizerImpl::dial() {
    auto address = peers_[peer_index_.load()];
    client_ = std::make_unique<DrandSyncClientImpl>(address);
  }
}  // namespace fc::drand
