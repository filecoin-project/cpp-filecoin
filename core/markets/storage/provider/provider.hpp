/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_MARKETS_STORAGE__PROVIDER_PROVIDER_HPP
#define CPP_FILECOIN_CORE_MARKETS_STORAGE__PROVIDER_PROVIDER_HPP

#include <memory>
#include <vector>

#include <libp2p/connection/stream.hpp>
#include "common/buffer.hpp"
#include "common/outcome.hpp"
#include "markets/storage/ask_protocol.hpp"
#include "markets/storage/deal_protocol.hpp"
#include "primitives/address/address.hpp"
#include "primitives/chain_epoch/chain_epoch.hpp"
#include "primitives/cid/cid.hpp"
#include "primitives/types.hpp"

namespace fc::markets::storage::provider {
  using common::Buffer;
  using primitives::ChainEpoch;
  using primitives::TokenAmount;
  using primitives::address::Address;

  class StorageProvider {
   public:
    virtual ~StorageProvider() = default;

    virtual auto init() -> outcome::result<void> = 0;

    virtual auto start() -> outcome::result<void> = 0;

    virtual auto stop() -> outcome::result<void> = 0;

    virtual auto addAsk(const TokenAmount &price, ChainEpoch duration)
        -> outcome::result<void> = 0;

    virtual auto listAsks(const Address &address)
        -> outcome::result<std::vector<SignedStorageAsk>> = 0;

    /**
     * Get deal by proposal cid
     * @param proposal_cid - proposal data cid
     * @return deal data
     */
    virtual auto getDeal(const CID &proposal_cid) const
        -> outcome::result<MinerDeal> = 0;

    virtual auto addStorageCollateral(const TokenAmount &amount)
        -> outcome::result<void> = 0;

    virtual auto getStorageCollateral() -> outcome::result<TokenAmount> = 0;

    virtual auto importDataForDeal(const CID &proposal_cid, const Buffer &data)
        -> outcome::result<void> = 0;
  };
}  // namespace fc::markets::storage::provider

#endif  // CPP_FILECOIN_CORE_MARKETS_STORAGE__PROVIDER_PROVIDER_HPP
