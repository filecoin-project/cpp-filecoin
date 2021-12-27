/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/asio.hpp>
#include "miner/miner.hpp"

#include "api/full_node/node_api.hpp"
#include "common/outcome.hpp"
#include "primitives/address/address.hpp"
#include "primitives/stored_counter/stored_counter.hpp"
#include "sector_storage/manager.hpp"
#include "storage/buffer_map.hpp"
#include "storage/chain/chain_store.hpp"

namespace fc::miner {
  using api::FullNodeApi;
  using libp2p::basic::Scheduler;
  using mining::DealInfo;
  using mining::PieceLocation;
  using mining::types::SectorInfo;
  using primitives::Counter;
  using primitives::SectorNumber;
  using primitives::address::Address;
  using primitives::piece::PieceData;
  using primitives::piece::UnpaddedPieceSize;
  using sector_storage::Manager;
  using storage::BufferMap;

  class MinerImpl : public Miner {
   public:
    static outcome::result<std::shared_ptr<MinerImpl>> newMiner(
        const std::shared_ptr<FullNodeApi> &api,
        const Address &miner_address,
        const Address &worker_address,
        const std::shared_ptr<Counter> &counter,
        const std::shared_ptr<BufferMap> &sealing_fsm_kv,
        const std::shared_ptr<Manager> &sector_manager,
        const std::shared_ptr<Scheduler> &scheduler,
        const std::shared_ptr<boost::asio::io_context> &context,
        const mining::Config &config,
        const std::vector<Address> &precommit_control);

    outcome::result<std::shared_ptr<SectorInfo>> getSectorInfo(
        SectorNumber sector_id) const override;

    outcome::result<PieceLocation> addPieceToAnySector(
        const UnpaddedPieceSize &size,
        PieceData piece_data,
        const DealInfo &deal) override;

    Address getAddress() const override;

    std::shared_ptr<Sealing> getSealing() const override;

   private:
    explicit MinerImpl(std::shared_ptr<Sealing> sealing);

    std::shared_ptr<Sealing> sealing_;
  };

}  // namespace fc::miner
