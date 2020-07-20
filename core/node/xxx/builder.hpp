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
  namespace sync {
    class Sync;
  }

  namespace clock {
    class UTCClock;
    class ChainEpochClock;
  }  // namespace clock

  namespace storage {
    namespace ipfs {
      class IpfsDatastore;

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
}  // namespace fc

namespace fc::node {

  struct NodeObjects {
    std::shared_ptr<boost::asio::io_context> io_context;
    std::shared_ptr<libp2p::protocol::Scheduler> scheduler;

    std::shared_ptr<libp2p::Host> host;

    std::shared_ptr<libp2p::protocol::Identify> identify_protocol;
    std::shared_ptr<libp2p::protocol::IdentifyPush> identify_push_protocol;
    std::shared_ptr<libp2p::protocol::IdentifyDelta> identify_delta_protocol;

    std::shared_ptr<clock::UTCClock> utc_clock;
    std::shared_ptr<clock::ChainEpochClock> chain_epoch_clock;

    std::shared_ptr<storage::ipfs::IpfsDatastore> ipfs_datastore;

    std::shared_ptr<blockchain::block_validator::BlockValidator>
        block_validator;
    std::shared_ptr<storage::blockchain::ChainStore> chain_store;

    std::shared_ptr<libp2p::protocol::gossip::Gossip> gossip;

    std::shared_ptr<storage::ipfs::graphsync::Graphsync> graphsync;

    std::shared_ptr<api::Api> api;
  };

  outcome::result<NodeObjects> createNodeObjects(const Config &config);

}  // namespace fc::node

#endif  // CPP_FILECOIN_SYNC_BUILDER_HPP
