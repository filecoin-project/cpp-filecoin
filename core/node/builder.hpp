/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_SYNC_BUILDER_HPP
#define CPP_FILECOIN_SYNC_BUILDER_HPP

#include <memory>

#include <boost/asio.hpp>

#include "common/outcome.hpp"
#include "config.hpp"
#include "storage/buffer_map.hpp"

// fwd declarations go here
namespace libp2p {
  class Host;

  namespace protocol {
    class Scheduler;
    class Identify;
    class IdentifyPush;
    class IdentifyDelta;

    namespace gossip {
      class Gossip;
    }
  }  // namespace protocol
}  // namespace libp2p

namespace fc {
  namespace clock {
    class UTCClock;
    class ChainEpochClock;
  }  // namespace clock

  namespace storage {
    namespace ipfs {
      class IpfsDatastore;
      class PersistentBufferMap;

      namespace graphsync {
        class Graphsync;
      }
    }  // namespace ipfs

    namespace blockchain {
      class ChainStore;
    }
  }  // namespace storage

  namespace blockchain {
    namespace block_validator {
      class BlockValidator;
    }
  }  // namespace blockchain

  namespace api {
    struct Api;
  }  // namespace api

  namespace sync {
    class TipsetLoader;
    class BlockLoader;
    class IndexDb;
    class IndexDbBackend;
    class ChainDb;
    class PeerManager;

    namespace blocksync {
      class BlocksyncClient;
    }
  }  // namespace sync

  namespace vm::interpreter {
    class Interpreter;
  }
}  // namespace fc

namespace fc::node {

  enum Error {
    STORAGE_INIT_ERROR = 1,
    CAR_FILE_OPEN_ERROR,
    CAR_FILE_SIZE_ABOVE_LIMIT,
    NO_GENESIS_BLOCK,
    GENESIS_MISMATCH,
  };

  struct NodeObjects {
    std::shared_ptr<
        storage::face::PersistentMap<common::Buffer, common::Buffer>>
        kvstorage;

    std::shared_ptr<storage::ipfs::IpfsDatastore> ipld;

    std::shared_ptr<boost::asio::io_context> io_context;

    std::shared_ptr<libp2p::protocol::Scheduler> scheduler;

    std::shared_ptr<libp2p::Host> host;

    std::shared_ptr<clock::UTCClock> utc_clock;

    std::shared_ptr<sync::IndexDbBackend> index_db_backend;

    std::shared_ptr<sync::IndexDb> index_db;

    std::shared_ptr<sync::ChainDb> chain_db;

    std::shared_ptr<sync::PeerManager> peer_manager;

    std::shared_ptr<sync::blocksync::BlocksyncClient> blocksync_client;

    std::shared_ptr<sync::BlockLoader> block_loader;

    std::shared_ptr<sync::TipsetLoader> tipset_loader;

    std::shared_ptr<vm::interpreter::Interpreter> vm_interpreter;

    // std::shared_ptr<libp2p::protocol::gossip::Gossip> gossip;

    // std::shared_ptr<storage::ipfs::graphsync::Graphsync> graphsync;

    // std::shared_ptr<storage::blockchain::ChainStore> chain_store;

    // std::shared_ptr<api::Api> api;

    // std::shared_ptr<clock::ChainEpochClock> chain_epoch_clock;
  };

  outcome::result<NodeObjects> createNodeObjects(Config &config);

}  // namespace fc::node

OUTCOME_HPP_DECLARE_ERROR(fc::node, Error);

#endif  // CPP_FILECOIN_SYNC_BUILDER_HPP
