/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/asio.hpp>
#include <memory>

#include "common/outcome.hpp"
#include "fwd.hpp"
#include "node/events.hpp"
#include "node/main/config.hpp"
#include "primitives/tipset/load.hpp"
#include "storage/buffer_map.hpp"
#include "storage/leveldb/leveldb.hpp"
#include "vm/runtime/env_context.hpp"

namespace fc::node {

  enum Error {
    kStorageInitError = 1,
    kCarOpenFileError,
    kCarFileAboveLimit,
    kNoGenesisBlock,
    kGenesisMismatch,
  };

  struct NodeObjects {
    // storage objects
    std::shared_ptr<storage::LevelDB> ipld_leveldb;
    std::shared_ptr<storage::ipfs::IpfsDatastore> ipld;
    std::shared_ptr<primitives::tipset::TsLoadIpld> ts_load_ipld;
    std::shared_ptr<primitives::tipset::TsLoadCache> ts_load;
    std::shared_ptr<storage::PersistentBufferMap> kv_store;
    std::shared_ptr<storage::PersistentBufferMap> ts_main_kv;
    TsBranchesPtr ts_branches;
    TsBranchPtr ts_main;

    // clocks
    std::shared_ptr<clock::UTCClock> utc_clock;
    std::shared_ptr<clock::ChainEpochClock> chain_epoch_clock;

    // libp2p + async base objects
    std::shared_ptr<boost::asio::io_context> io_context;
    std::shared_ptr<libp2p::protocol::Scheduler> scheduler;
    std::shared_ptr<sync::events::Events> events;
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
    std::shared_ptr<vm::interpreter::InterpreterImpl> interpreter;
    std::shared_ptr<vm::interpreter::CachedInterpreter> vm_interpreter;
    std::shared_ptr<sync::SyncJob> sync_job;
    vm::runtime::EnvironmentContext env_context;

    // high level objects
    std::shared_ptr<sync::ChainStoreImpl> chain_store;
    std::shared_ptr<api::FullNodeApi> api;
  };

  outcome::result<NodeObjects> createNodeObjects(Config &config);
}  // namespace fc::node

OUTCOME_HPP_DECLARE_ERROR(fc::node, Error);