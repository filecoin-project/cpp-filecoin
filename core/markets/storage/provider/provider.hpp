/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_MARKETS_STORAGE_PROVIDER_PROVIDER_HPP
#define CPP_FILECOIN_MARKETS_STORAGE_PROVIDER_PROVIDER_HPP

#include "markets/storage/provider.hpp"

namespace fc::markets::storage {

  class StorageProviderImpl : public StorageProvider {
   public:
    virtual auto addAsk(const TokenAmount &price, ChainEpoch duration)
        -> outcome::result<void>;

    virtual auto listAsks(const Address &address)
        -> outcome::result<std::vector<SignedStorageAsk>>;

    virtual auto listDeals() -> outcome::result<std::vector<StorageDeal>>;

    virtual auto listIncompleteDeals()
        -> outcome::result<std::vector<MinerDeal>>;

    virtual auto addStorageCollaterial(const TokenAmount &amount)
        -> outcome::result<void>;

    virtual auto getStorageCollaterial() -> outcome::result<TokenAmount>;

    virtual auto importDataForDeal(const CID &prop_cid,
                                   const libp2p::connection::Stream &data)
        -> outcome::result<void>;

   private:
  };

}  // namespace fc::markets::storage

#endif  // CPP_FILECOIN_MARKETS_STORAGE_PROVIDER_PROVIDER_HPP
