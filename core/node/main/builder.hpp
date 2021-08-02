/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/asio.hpp>
#include <memory>

#include "api/full_node/node_api.hpp"
#include "api/full_node/node_api_v1_wrapper.hpp"
#include "api/rpc/json.hpp"
#include "common/outcome.hpp"
#include "data_transfer/dt.hpp"
#include "fwd.hpp"
#include "markets/discovery/discovery.hpp"
#include "markets/retrieval/client/retrieval_client.hpp"
#include "markets/storage/chain_events/impl/chain_events_impl.hpp"
#include "markets/storage/client/import_manager/import_manager.hpp"
#include "markets/storage/client/storage_market_client.hpp"
#include "node/events.hpp"
#include "node/main/config.hpp"
#include "primitives/tipset/load.hpp"
#include "storage/buffer_map.hpp"
#include "storage/car/cids_index/cids_index.hpp"
#include "storage/compacter/compacter.hpp"
#include "storage/ipfs/impl/datastore_leveldb.hpp"
#include "storage/ipld/cids_ipld.hpp"
#include "storage/keystore/keystore.hpp"
#include "storage/leveldb/leveldb.hpp"
#include "storage/leveldb/prefix.hpp"
#include "vm/runtime/env_context.hpp"

namespace fc::node {
  using api::FullNodeApiV1Wrapper;
  using api::KeyInfo;
  using data_transfer::DataTransfer;
  using libp2p::basic::Scheduler;
  using markets::discovery::Discovery;
  using markets::retrieval::client::RetrievalClient;
  using markets::storage::chain_events::ChainEventsImpl;
  using markets::storage::client::StorageMarketClient;
  using markets::storage::client::import_manager::ImportManager;
  using primitives::address::Address;
  using storage::OneKey;
  using storage::keystore::KeyStore;

  enum Error {
    kStorageInitError = 1,
    kCarOpenFileError,
    kCarFileAboveLimit,
    kNoGenesisBlock,
    kGenesisMismatch,
  };

  struct NodeObjects {
    // storage objects
    std::shared_ptr<storage::LevelDB> ipld_leveldb_kv;
    std::shared_ptr<storage::ipfs::LeveldbDatastore> ipld_leveldb;
    IpldPtr markets_ipld;
    std::shared_ptr<storage::ipld::CidsIpld> ipld_cids;
    std::shared_ptr<IoThread> ipld_flush_thread;
    std::shared_ptr<storage::compacter::CompacterIpld> compacter;
    IpldPtr ipld;
    std::shared_ptr<primitives::tipset::TsLoadIpld> ts_load_ipld;
    std::shared_ptr<primitives::tipset::TsLoadCache> ts_load;
    std::shared_ptr<storage::PersistentBufferMap> kv_store;
    TsBranchesPtr ts_branches;
    TsBranchPtr ts_main;
    std::shared_ptr<storage::mpool::MessagePool> mpool;

    // clocks
    std::shared_ptr<clock::UTCClock> utc_clock;
    std::shared_ptr<clock::ChainEpochClock> chain_epoch_clock;

    // libp2p + async base objects
    std::shared_ptr<boost::asio::io_context> io_context;
    std::shared_ptr<Scheduler> scheduler;
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
    std::shared_ptr<vm::interpreter::Interpreter> vm_interpreter;
    std::shared_ptr<sync::SyncJob> sync_job;
    vm::runtime::EnvironmentContext env_context;

    // markets
    std::shared_ptr<DataTransfer> datatransfer;
    std::shared_ptr<ImportManager> storage_market_import_manager;
    std::shared_ptr<ChainEventsImpl> chain_events;
    std::shared_ptr<Discovery> market_discovery;
    std::shared_ptr<StorageMarketClient> storage_market_client;
    std::shared_ptr<RetrievalClient> retrieval_market_client;

    std::shared_ptr<KeyStore> key_store;
    std::shared_ptr<OneKey> wallet_default_address;

    // high level objects
    std::shared_ptr<sync::ChainStoreImpl> chain_store;
    // Full node API v1.x.x
    std::shared_ptr<api::FullNodeApiV1Wrapper> api_v1;
    // Full node API v2.x.x (latest)
    std::shared_ptr<api::FullNodeApi> api;
  };

  /**
   * Reads private key from file to import as default key
   * @param path to the file
   * @return KeyInfo
   */
  outcome::result<KeyInfo> readPrivateKeyFromFile(const std::string &path);

  outcome::result<NodeObjects> createNodeObjects(Config &config);
}  // namespace fc::node

OUTCOME_HPP_DECLARE_ERROR(fc::node, Error);
