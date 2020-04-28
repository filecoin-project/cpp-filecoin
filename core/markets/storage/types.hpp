/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_MARKETS_STORAGE_TYPES_HPP
#define CPP_FILECOIN_MARKETS_STORAGE_TYPES_HPP

#include <libp2p/peer/peer_id.hpp>
#include "primitives/address/address.hpp"
#include "primitives/cid/cid.hpp"
#include "primitives/types.hpp"

namespace fc::markets::storage {

  using libp2p::peer::PeerId;
  using primitives::SectorSize;
  using primitives::address::Address;

  struct ProposeStorageDealResult {
    CID proposal_cid;
  };

  // Closely follows the MinerInfo struct in the spec
  struct StorageProviderInfo {
    Address address;  // actor address
    Address owner;
    Address worker;  // signs messages
    SectorSize sector_size;
    PeerId peer_id;
  };

}  // namespace fc::markets::storage

#endif  // CPP_FILECOIN_MARKETS_STORAGE_TYPES_HPP
