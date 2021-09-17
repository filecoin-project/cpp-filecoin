/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <memory>
#include <set>
#include <shared_mutex>

#include "const.hpp"

namespace boost {
  namespace asio {
    class io_context;
  }  // namespace asio
}  // namespace boost

namespace libp2p {
  struct Host;

  namespace basic {
    // A class with the same name is present in fc::sector_storage
    // NOLINTNEXTLINE(bugprone-forward-declaration-namespace)
    class Scheduler;
  }

  namespace connection {
    class Stream;
  }  // namespace connection

  namespace peer {
    class PeerId;
    class PeerInfo;
  }  // namespace peer

  namespace protocol {
    // A class with the same name is present in fc::sync
    // NOLINTNEXTLINE(bugprone-forward-declaration-namespace)
    class Identify;
    class IdentifyPush;
    class IdentifyDelta;

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
    struct FullNodeApi;
  }  // namespace api

  namespace blockchain {
    namespace block_validator {
      class BlockValidator;
    }  // namespace block_validator

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
      struct TsLoad;

      using TipsetCPtr = std::shared_ptr<const Tipset>;
      using TsLoadPtr = std::shared_ptr<TsLoad>;
      using TsWeak = std::weak_ptr<const Tipset>;

      namespace chain {
        struct TsBranch;

        using TsBranchPtr = std::shared_ptr<TsBranch>;
        using TsBranches = std::set<TsBranchPtr>;
        using TsBranchesPtr = std::shared_ptr<TsBranches>;
      }  // namespace chain
    }    // namespace tipset
  }      // namespace primitives

  namespace storage {
    namespace blockchain {
      class ChainStore;
    }  // namespace blockchain

    namespace ipfs {
      struct IpfsDatastore;

      namespace graphsync {
        class Graphsync;
      }  // namespace graphsync
    }    // namespace ipfs
  }      // namespace storage

  namespace sync {
    class ChainStoreImpl;
    class GraphsyncServer;
    // A class with the same name is present in libp2p::protocol
    // NOLINTNEXTLINE(bugprone-forward-declaration-namespace)
    class Identify;
    class PeerDiscovery;
    class PubSubGate;
    class ReceiveHello;
    class SayHello;
    class SyncJob;

    namespace blocksync {
      class BlocksyncServer;
    }  // namespace blocksync
  }    // namespace sync

  namespace vm {
    struct Circulating;

    namespace actor {
      struct Actor;
      class Invoker;
    }  // namespace actor

    namespace interpreter {
      class CachedInterpreter;
      class Interpreter;
      struct InterpreterCache;
      class InterpreterImpl;
    }  // namespace interpreter

    namespace message {
      struct SignedMessage;
      struct UnsignedMessage;
    }  // namespace message

    namespace runtime {
      struct Execution;
      struct MessageReceipt;
      class Runtime;
      class RuntimeRandomness;
    }  // namespace runtime

    namespace state {
      class StateTree;
      class StateTreeImpl;
    }  // namespace state
  }    // namespace vm
}  // namespace fc

namespace fc {
  using primitives::tipset::TsLoadPtr;
  using primitives::tipset::chain::TsBranch;
  using primitives::tipset::chain::TsBranches;
  using primitives::tipset::chain::TsBranchesPtr;
  using primitives::tipset::chain::TsBranchPtr;

  using Ipld = storage::ipfs::IpfsDatastore;
  using IpldPtr = std::shared_ptr<Ipld>;
  using SharedMutexPtr = std::shared_ptr<std::shared_mutex>;
}  // namespace fc
