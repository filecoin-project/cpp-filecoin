/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_MARKETS_STORAGE_TYPES_HPP
#define CPP_FILECOIN_MARKETS_STORAGE_TYPES_HPP

#include "primitives/address/address.hpp"
#include "primitives/cid/cid.hpp"
#include "primitives/sector/types.hpp"

namespace fc::markets::storage {

  using primitives::SectorSize;
  using primitives::address::Address;
  using PeerId = std::string;

  struct ProposeStorageDealResult {
    CID proposal_cid;
  };

  // Closely follows the MinerInfo struct in the spec
  struct StorageProviderInfo {
    Address address;  // actor address
    Address owner;
    Address worker;  // signs messages
    SectorSize sector_size;
    PeerID peer_id;
  }

}  // namespace fc::markets::storage

#endif  // CPP_FILECOIN_MARKETS_STORAGE_TYPES_HPP
