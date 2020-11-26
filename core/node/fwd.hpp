/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_NODE_FWD_HPP
#define CPP_FILECOIN_NODE_FWD_HPP

#include <boost/signals2.hpp>

namespace libp2p {
  struct Host;

  namespace connection {
    class Stream;
  }  // namespace connection

  namespace peer {
    class PeerId;
    class PeerInfo;
  }  // namespace peer

  namespace protocol {
    class Scheduler;

    namespace gossip {
      class Gossip;
    }  // namespace gossip
  }    // namespace protocol
}  // namespace libp2p

namespace fc {
  class CID;

  namespace api {
    struct Api;
  }  // namespace api

  namespace clock {
    class UTCClock;
    class ChainEpochClock;
  }  // namespace clock

  namespace crypto {
    namespace bls {
      class BlsProvider;
    }  // namespace bls
  }    // namespace crypto

  namespace data_transfer {
    class GraphsyncReceiver;
  }  // namespace data_transfer

  namespace drand {
    class Beaconizer;
    struct DrandSchedule;
  }  // namespace drand

  namespace markets {
    namespace storage {
      namespace provider {
        class StoredAsk;
      }  // namespace provider
    }    // namespace storage
  }      // namespace markets

  namespace primitives {
    namespace address {
      struct Address;
    }  // namespace address

    namespace block {
      struct BlockWithCids;
    }  // namespace block

    namespace tipset {
      struct Tipset;
    }  // namespace tipset
  }    // namespace primitives

  namespace storage {
    namespace blockchain {
      class ChainStore;
    }  // namespace blockchain

    namespace ipfs {
      class IpfsDatastore;

      namespace graphsync {
        class Graphsync;
      }  // namespace graphsync
    }    // namespace ipfs
  }      // namespace storage

  namespace vm {
    namespace actor {
      struct Actor;
    }  // namespace actor

    namespace interpreter {
      class Interpreter;
    }  // namespace interpreter

    namespace message {
      struct SignedMessage;
      struct UnsignedMessage;
    }  // namespace message

    namespace runtime {
      struct Execution;
      struct MessageReceipt;
    }  // namespace runtime

    namespace state {
      class StateTree;
      class StateTreeImpl;
    }  // namespace state
  }    // namespace vm
}  // namespace fc

namespace fc {
  using Ipld = storage::ipfs::IpfsDatastore;
  using IpldPtr = std::shared_ptr<Ipld>;

  namespace sync {
    namespace events {
      using Connection = boost::signals2::scoped_connection;

      struct Events;

      // event types
      struct PeerConnected;
      struct PeerDisconnected;
      struct PeerLatency;
      struct TipsetFromHello;
      struct BlockFromPubSub;
      struct MessageFromPubSub;
      struct BlockStored;
      struct TipsetStored;
      struct PossibleHead;
      struct HeadInterpreted;
      struct CurrentHead;
    }  // namespace events
  }    // namespace sync
}  // namespace fc

#endif  // CPP_FILECOIN_NODE_FWD_HPP
