/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <memory>

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
  }  // namespace protocol
}  // namespace libp2p

namespace fc {
  class CID;

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

  namespace storage {
    namespace blockchain {
      class ChainStore;
    }  // namespace blockchain

    namespace ipfs {
      class IpfsDatastore;
    }  // namespace ipfs
  }    // namespace storage

  namespace vm::interpreter {
    class Interpreter;
  }  // namespace vm::interpreter
}  // namespace fc

namespace fc {
  using Ipld = storage::ipfs::IpfsDatastore;
  using IpldPtr = std::shared_ptr<Ipld>;
}  // namespace fc
