/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/math/math.hpp"
#include "const.hpp"
#include "primitives/tipset/tipset.hpp"
#include "storage/keystore/keystore.hpp"

namespace fc::primitives::block {
  using common::math::expneg;
  using common::math::kPrecision256;
  using crypto::randomness::DomainSeparationTag;
  using crypto::randomness::drawRandomness;
  using crypto::randomness::Randomness;
  using primitives::tipset::Tipset;

  struct BlockRand {
    Randomness election;
    Randomness ticket;
    Randomness win;

    BlockRand(Address miner,
              ChainEpoch height,
              const std::vector<BeaconEntry> &beacons,
              const BeaconEntry &prev_beacon,
              const Tipset &parent) {
      const auto &beacon{beacons.empty() ? prev_beacon : beacons.back()};
      const auto miner_seed{codec::cbor::encode(miner).value()};
      election = drawRandomness(beacon.data,
                                DomainSeparationTag::ElectionProofProduction,
                                height,
                                miner_seed);
      auto ticket_seed{miner_seed};
      if (height > kUpgradeSmokeHeight) {
        append(ticket_seed, parent.getMinTicketBlock().ticket->bytes);
      }
      constexpr auto kTicketRandomnessLookback{1};
      ticket = drawRandomness(beacon.data,
                              DomainSeparationTag::TicketProduction,
                              height - kTicketRandomnessLookback,
                              ticket_seed);
      win = drawRandomness(beacon.data,
                           DomainSeparationTag::WinningPoStChallengeSeed,
                           height,
                           miner_seed);
    }
  };

  inline BigInt blakeBigInt(BytesIn vrf) {
    const auto _hash{crypto::blake2b::blake2b_256(vrf)};
    BigInt hash;
    boost::multiprecision::import_bits(hash, _hash.begin(), _hash.end());
    return hash;
  }

  inline int64_t computeWinCount(BytesIn election_vrf,
                                 const BigInt &power,
                                 const BigInt &total_power) {
    const auto hash{blakeBigInt(election_vrf)};
    const auto lambda{
        bigdiv((power * kBlocksPerEpoch) << kPrecision256, total_power)};
    auto pmf{expneg(lambda, kPrecision256)};
    BigInt icdf{(BigInt{1} << kPrecision256) - pmf};
    auto k{0};
    constexpr auto kMaxWinCount{3 * kBlocksPerEpoch};
    while (hash < icdf && k < kMaxWinCount) {
      ++k;
      pmf = (bigdiv(pmf, k) * lambda) >> kPrecision256;
      icdf -= pmf;
    }
    return k;
  }

  inline outcome::result<bool> checkBlockSignature(const BlockHeader &block,
                                                   const Address &worker) {
    if (!block.block_sig) {
      return false;
    }
    auto block2{block};
    block2.block_sig.reset();
    OUTCOME_TRY(data, codec::cbor::encode(block2));
    return storage::keystore::kDefaultKeystore->verify(
        worker, data, *block.block_sig);
  }
}  // namespace fc::primitives::block

namespace fc {
  using primitives::block::blakeBigInt;
  using primitives::block::BlockRand;
  using primitives::block::checkBlockSignature;
  using primitives::block::computeWinCount;
}  // namespace fc
