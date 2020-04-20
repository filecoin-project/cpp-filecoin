/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_MARKETS_STORAGE_CLIENT_HPP
#define CPP_FILECOIN_CORE_MARKETS_STORAGE_CLIENT_HPP

#include "common/outcome.hpp"
#include "markets/storage/ask_protocol.hpp"
#include "markets/storage/deal_protocol.hpp"
#include "markets/storage/types.hpp"
#include "primitives/address/address.hpp"
#include "primitives/cid/cid.hpp"
#include "primitives/sector/sector.hpp"
#include "storage/filestore/filestore.hpp"

namespace fc::markets::storage {

  using fc::storage::filestore::FileStore;
  using primitives::address::Address;
  using primitives::sector::RegisteredProof;

  class Client {
   public:
    virtual ~Client() = defaul;

    virtual void run() = 0;

    virtual void stop() = 0;

    virtual outcome::result<std::vector<StorageProviderInfo>> listProviders()
        const = 0;

    virtual outcome::result<std::vector<StorageDeal>> listDeals(
        const Address &address) const = 0;

    virtual outcome::result<std::vector<StorageDeal>> listLocalDeals()
        const = 0;

    virtual outcome::result<StorageDeal> getLocalDeal(const CID &cid) const = 0;

    virtual outcome::result<SignedStorageAsk> getAsk(
        const StorageProviderInfo &info) const = 0;

    virtual outcome::result<ProposeStorageDealResult> proposeStorageDeal(
        const Address &address,
        const StorageProviderInfo &provider_info,
        const DataRef &data_ref,
        const ChainEpoch &start_epoch,
        const ChainEpoch &end_epoch,
        const TokenAmount &price,
        const TokenAmount &collateral,
        const RegisteredProof &registered_proof) = 0;

    virtual outcome::result<StorageParticipantBalance> getPaymentEscrow(
        const Address &address) const = 0;

    virtual outcome::result<void> addPaymentEscrow(
        const Address &address, const TokenAmount &amount) = 0;
  };

}  // namespace fc::markets::storage

#endif  // CPP_FILECOIN_CORE_MARKETS_STORAGE_CLIENT_HPP
