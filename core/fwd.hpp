/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <memory>

namespace boost {
  namespace asio {
    class io_context;
  }  // namespace asio
}  // namespace boost

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
    class Identify;
    class IdentifyPush;
    class IdentifyDelta;
    class Scheduler;

    namespace gossip {
      class Gossip;
    }  // namespace gossip

    namespace kademlia {
      class Kademlia;
    }  // namespace kademlia
  }    // namespace protocol
}  // namespace libp2p

namespace fc {
  class CID;

  namespace api {
    struct Api;
  }  // namespace api

  namespace blockchain {
    namespace weight {
      class WeightCalculator;
    }  // namespace weight
  }    // namespace blockchain

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

  namespace hello {
    struct Hello;
  }  // namespace hello

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
      using TipsetCPtr = std::shared_ptr<const Tipset>;
    }  // namespace tipset
  }    // namespace primitives

  namespace pubsub {
    struct PubSub;
  }  // namespace pubsub

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
      class Runtime;
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
}  // namespace fc
