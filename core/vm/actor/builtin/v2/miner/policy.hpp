/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "primitives/sector/sector.hpp"

namespace fc::vm::actor::builtin::v2::miner {
  using primitives::sector::RegisteredSealProof;

  /** Maximum number of control addresses a miner may register. */
  constexpr size_t kMaxControlAddresses = 10;

  /**
   * List of proof types which may be used when creating a new miner actor or
   * pre-committing a new sector.
   */
  static const std::set<RegisteredSealProof> kPreCommitSealProofTypesV0{
      RegisteredSealProof::StackedDrg32GiBV1,
      RegisteredSealProof::StackedDrg64GiBV1,
  };

  static const std::set<RegisteredSealProof> kPreCommitSealProofTypesV7{
      RegisteredSealProof::StackedDrg32GiBV1,
      RegisteredSealProof::StackedDrg64GiBV1,
      RegisteredSealProof::StackedDrg32GiBV1_1,
      RegisteredSealProof::StackedDrg64GiBV1_1,
  };

  /**
   * From network version 8, sectors sealed with the V1 seal proof types cannot
   * be committed
   */
  static const std::set<RegisteredSealProof> kPreCommitSealProofTypesV8{
      RegisteredSealProof::StackedDrg32GiBV1_1,
      RegisteredSealProof::StackedDrg64GiBV1_1,
  };

  /// Libp2p peer info limits.
  /**
   * MaxPeerIDLength is the maximum length allowed for any on-chain peer ID.
   * Most Peer IDs are expected to be less than 50 bytes.
   */
  constexpr size_t kMaxPeerIDLength = 128;

  /**
   * MaxMultiaddrData is the maximum amount of data that can be stored in
   * multiaddrs.
   */
  constexpr size_t kMaxMultiaddressData = 1024;

}  // namespace fc::vm::actor::builtin::v2::miner
