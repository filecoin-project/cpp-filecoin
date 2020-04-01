/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_MARKETS_STORAGE__PROVIDER_PROVIDER_HPP
#define CPP_FILECOIN_CORE_MARKETS_STORAGE__PROVIDER_PROVIDER_HPP

#include <vector>

#include <libp2p/connection/stream.hpp>

#include "common/outcome.hpp"
#include "markets/storage/ask_protocol.hpp"
#include "markets/storage/deal_protocol.hpp"
#include "primitives/address/address.hpp"
#include "primitives/chain_epoch/chain_epoch.hpp"
#include "primitives/cid/cid.hpp"
#include "primitives/types.hpp"

namespace fc::markets::storage {
  using primitives::ChainEpoch;
  using primitives::TokenAmount;
  using primitives::address::Address;

  class StorageProvider {
   public:
    virtual ~StorageProvider() = default;

    virtual auto addAsk(const TokenAmount &price, ChainEpoch duration)
        -> outcome::result<void> = 0;

    virtual auto listAsks(const Address &address)
        -> outcome::result<std::vector<SignedStorageAsk>> = 0;

    virtual auto listDeals() -> outcome::result<std::vector<StorageDeal>> = 0;

    virtual auto listIncompleteDeals()
        -> outcome::result<std::vector<MinerDeal>> = 0;

    virtual auto addStorageCollaterial(const TokenAmount &amount)
        -> outcome::result<void> = 0;

    virtual auto getStorageCollaterial() -> outcome::result<TokenAmount> = 0;

    virtual auto importDataForDeal(const CID &prop_cid,
                                   const libp2p::connection::Stream &data)
        -> outcome::result<void> = 0;
  };
}  // namespace fc::markets::storage

#endif  // CPP_FILECOIN_CORE_MARKETS_STORAGE__PROVIDER_PROVIDER_HPP
