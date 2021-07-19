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
  using mining::DealInfo;
  using mining::PieceAttributes;
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
        std::shared_ptr<FullNodeApi> api,
        Address miner_address,
        Address worker_address,
        std::shared_ptr<Counter> counter,
        std::shared_ptr<BufferMap> sealing_fsm_kv,
        std::shared_ptr<Manager> sector_manager,
        std::shared_ptr<libp2p::protocol::Scheduler> scheduler,
        std::shared_ptr<boost::asio::io_context> context,
        mining::Config config);

    outcome::result<std::shared_ptr<SectorInfo>> getSectorInfo(
        SectorNumber sector_id) const override;

    outcome::result<PieceAttributes> addPieceToAnySector(
        UnpaddedPieceSize size,
        PieceData piece_data,
        DealInfo deal) override;

    Address getAddress() const override;

    std::shared_ptr<Sealing> getSealing() const override;

   private:
    MinerImpl(std::shared_ptr<FullNodeApi> api,
              std::shared_ptr<Sealing> sealing);

    std::shared_ptr<FullNodeApi> api_;
    std::shared_ptr<Sealing> sealing_;
  };

}  // namespace fc::miner
