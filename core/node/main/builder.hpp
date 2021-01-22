/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_SYNC_BUILDER_HPP
#define CPP_FILECOIN_SYNC_BUILDER_HPP

#include <memory>

#include <boost/asio.hpp>

#include "common/outcome.hpp"
#include "fwd.hpp"
#include "node/main/config.hpp"
#include "storage/buffer_map.hpp"

// fwd declarations go here
namespace libp2p {
  struct Host;

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
    class ChainStoreImpl;
    class IndexDb;
    class ChainDb;
    class Identify;
    class SayHello;
    class ReceiveHello;
    class PubSubGate;
    class PeerDiscovery;
    class GraphsyncServer;
    class SyncJob;
    class InterpretJob;

    namespace blocksync {
      class BlocksyncServer;
    }  // namespace blocksync
  }    // namespace sync

  namespace vm::interpreter {
    class Interpreter;
  }

  namespace drand {
    class Beaconizer;
    struct DrandSchedule;
  }  // namespace drand
}  // namespace fc

namespace fc::node {

  enum Error {
    STORAGE_INIT_ERROR = 1,
    KEY_READ_ERROR,
    KEY_WRITE_ERROR,
    CAR_FILE_OPEN_ERROR,
    CAR_FILE_SIZE_ABOVE_LIMIT,
    NO_GENESIS_BLOCK,
    GENESIS_MISMATCH,
  };

  struct NodeObjects {
    // storage objects
    std::shared_ptr<storage::ipfs::IpfsDatastore> ipld;
    std::shared_ptr<storage::PersistentBufferMap> kv_store;
    std::shared_ptr<sync::IndexDb> index_db;
    std::shared_ptr<sync::ChainDb> chain_db;

    // clocks
    std::shared_ptr<clock::UTCClock> utc_clock;
    std::shared_ptr<clock::ChainEpochClock> chain_epoch_clock;

    // libp2p + async base objects
    std::shared_ptr<boost::asio::io_context> io_context;
    std::shared_ptr<libp2p::protocol::Scheduler> scheduler;
    std::shared_ptr<libp2p::Host> host;

    // base protocols
    std::shared_ptr<sync::Identify> identify;
    std::shared_ptr<sync::SayHello> say_hello;
    std::shared_ptr<sync::ReceiveHello> receive_hello;

    // peer discovery
    std::shared_ptr<sync::PeerDiscovery> peer_discovery;

    // pubsub
    std::shared_ptr<libp2p::protocol::gossip::Gossip> gossip;
    std::shared_ptr<sync::PubSubGate> pubsub_gate;

    // graphsync
    std::shared_ptr<storage::ipfs::graphsync::Graphsync> graphsync;
    std::shared_ptr<sync::GraphsyncServer> graphsync_server;

    // chain sync components
    std::shared_ptr<sync::blocksync::BlocksyncServer> blocksync_server;
    std::shared_ptr<vm::interpreter::Interpreter> vm_interpreter;
    std::shared_ptr<sync::SyncJob> sync_job;
    std::shared_ptr<sync::InterpretJob> interpret_job;

    // high level objects
    std::shared_ptr<sync::ChainStoreImpl> chain_store;
    std::shared_ptr<api::Api> api;
  };

  outcome::result<NodeObjects> createNodeObjects(Config &config);

  static constexpr auto kIndexDbFileName = "sqlite.db";
  static constexpr auto kLeveldbPath = "leveldb";
  static constexpr auto kPeerKeyPath = "peer_ed25519.key";

  IpldPtr makeIpld(std::shared_ptr<storage::BufferMap> map);
}  // namespace fc::node

OUTCOME_HPP_DECLARE_ERROR(fc::node, Error);

#endif  // CPP_FILECOIN_SYNC_BUILDER_HPP
