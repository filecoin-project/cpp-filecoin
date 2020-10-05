/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_MINER_MINER_HPP
#define CPP_FILECOIN_MINER_MINER_HPP

#include <boost/asio.hpp>

#include "api/api.hpp"
#include "common/outcome.hpp"
#include "miner/storage_fsm/sealing.hpp"
#include "primitives/address/address.hpp"
#include "primitives/stored_counter/stored_counter.hpp"
#include "sector_storage/manager.hpp"
#include "storage/chain/chain_store.hpp"

namespace fc::miner {
  using api::Api;
  using mining::Sealing;
  using mining::types::SectorInfo;
  using primitives::Counter;
  using primitives::SectorNumber;
  using primitives::address::Address;
  using sector_storage::Manager;

  class Miner {
   public:
    Miner(std::shared_ptr<Api> api,
          Address miner_address,
          Address worker_address,
          std::shared_ptr<Counter> counter,
          std::shared_ptr<Manager> sector_manager,
          std::shared_ptr<boost::asio::io_context> context);

    outcome::result<void> run();

    void stop();

    outcome::result<std::shared_ptr<SectorInfo>> getSectorInfo(
        SectorNumber sector_id) const;

   private:
    /**
     * Checks miner worker address
     * @return error if worker address is incorrect
     */
    outcome::result<void> runPreflightChecks();

    std::shared_ptr<Api> api_;
    Address miner_address_;
    Address worker_address_;
    std::shared_ptr<Sealing> sealing_;
    std::shared_ptr<Counter> counter_;
    std::shared_ptr<Manager> sector_manager_;
    std::shared_ptr<boost::asio::io_context> context_;
  };

  enum class MinerError {
    kWorkerNotFound = 1,
  };

}  // namespace fc::miner

OUTCOME_HPP_DECLARE_ERROR(fc::miner, MinerError);

#endif  // CPP_FILECOIN_MINER_MINER_HPP
