/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/filesystem/path.hpp>
#include <libp2p/connection/stream.hpp>
#include <string>

#include "common/outcome.hpp"
#include "markets/storage/mk_protocol.hpp"
#include "markets/storage/provider/miner_deal.hpp"
#include "markets/storage/provider/stored_ask.hpp"
#include "primitives/cid/cid.hpp"

namespace fc::markets::storage::provider {

  class StorageProvider {
   public:
    virtual ~StorageProvider() = default;

    virtual auto init() -> outcome::result<void> = 0;

    virtual auto start() -> outcome::result<void> = 0;

    virtual auto stop() -> outcome::result<void> = 0;

    /**
     * Get deal by proposal cid
     * @param proposal_cid - proposal data cid
     * @return deal data
     */
    virtual auto getDeal(const CID &proposal_cid) const
        -> outcome::result<MinerDeal> = 0;

    virtual auto getLocalDeals() const
        -> outcome::result<std::vector<MinerDeal>> = 0;

    /**
     * Imports data to proceed deal with 'manual' transfer type
     * @param proposal_cid - deal proposal CID
     * @param path - path to piece data file for the deal
     */
    virtual auto importDataForDeal(const CID &proposal_cid,
                                   const boost::filesystem::path &path)
        -> outcome::result<void> = 0;
  };

  void serveAsk(libp2p::Host &host, std::weak_ptr<StoredAsk> _asker);
}  // namespace fc::markets::storage::provider
