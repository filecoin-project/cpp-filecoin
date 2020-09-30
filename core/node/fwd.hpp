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
  class Host;

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
  }    // namespace protocol
}  // namespace libp2p

namespace fc {
  class CID;

  namespace clock {
    class UTCClock;
  }  // namespace clock

  namespace crypto {
    namespace bls {
      class BlsProvider;
    }  // namespace bls
  }    // namespace crypto

  namespace drand {
    class Beaconizer;
    struct DrandSchedule;
  }  // namespace drand

  namespace hello {
    struct Hello;
  }  // namespace hello

  namespace primitives {
    namespace block {
      struct BlockWithCids;
    }  // namespace block

    namespace tipset {
      struct Tipset;
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
    }  // namespace ipfs
  }    // namespace storage

  namespace vm {
    namespace interpreter {
      class Interpreter;
    }  // namespace interpreter

    namespace message {
      struct SignedMessage;
      struct UnsignedMessage;
    }  // namespace message

    namespace runtime {
      struct Execution;
    }  // namespace runtime
  }    // namespace vm
}  // namespace fc

namespace fc {
  using Ipld = storage::ipfs::IpfsDatastore;
  using IpldPtr = std::shared_ptr<Ipld>;
}  // namespace fc
