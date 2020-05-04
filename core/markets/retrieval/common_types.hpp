/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_MARKETS_RETRIEVAL_COMMON_TYPES_HPP
#define CPP_FILECOIN_CORE_MARKETS_RETRIEVAL_COMMON_TYPES_HPP

#include <libp2p/peer/peer_info.hpp>
#include "primitives/address/address.hpp"

namespace fc::markets::retrieval {
  using libp2p::peer::PeerId;
  using primitives::address::Address;

  /**
   * @struct Everything needed to make a deal with a miner
   */
  struct RetrievalPeer {
    Address address;
    PeerId id;

    inline bool operator==(const RetrievalPeer &rhs) const {
      return address == rhs.address && id == rhs.id;
    }
  };

}  // namespace fc::markets::retrieval

#endif  // CPP_FILECOIN_CORE_MARKETS_RETRIEVAL_COMMON_TYPES_HPP
