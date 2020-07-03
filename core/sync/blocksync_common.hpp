/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_SYNC_BLOCKSYNC_COMMON_HPP
#define CPP_FILECOIN_SYNC_BLOCKSYNC_COMMON_HPP

#include "codec/cbor/cbor.hpp"

#include "common.hpp"

namespace fc::storage::ipfs {
  class IpfsDataStore;
}

namespace fc::sync::blocksync {

  enum RequestOptions {
    BLOCKS_ONLY = 1,
    MESSAGES_ONLY = 2,
    BLOCKS_AND_MESSAGES = 3,
  };

  struct Request {
    std::vector<CID> block_cids;
    uint64_t depth = 1;
    RequestOptions options = BLOCKS_AND_MESSAGES;
  };

  using MsgIncudes = std::vector<std::vector<uint64_t>>;

  struct TipsetBundle {
    // TODO use not so heap consuming containers, like small_vector

    std::vector<BlockHeader> blocks;
    std::vector<UnsignedMessage> bls_msgs;
    MsgIncudes bls_msg_includes;
    std::vector<SignedMessage> secp_msgs;
    MsgIncudes secp_msg_includes;
  };

  enum class ResponseStatus : int {
    RESPONSE_COMPLETE = 0,
    RESPONSE_PARTIAL = 101,
    BLOCK_NOT_FOUND = 201,
    GO_AWAY = 202,
    INTERNAL_ERROR = 203,
    BAD_REQUEST = 204,
  };

  struct Response {
    std::vector<TipsetBundle> chain;
    ResponseStatus status;
    std::string message;
  };

  CBOR_TUPLE(Request, block_cids, depth, options);
  CBOR_TUPLE(TipsetBundle,
             blocks,
             bls_msgs,
             bls_msg_includes,
             secp_msgs,
             secp_msg_includes);
  CBOR_TUPLE(Response, chain, status, message);

  using OnBlockStored =
      std::function<void(CID block_cid, outcome::result<BlockMsg>)>;

  outcome::result<void> storeResponse(
      const std::shared_ptr<storage::ipfs::IpfsDatastore> &ipld,
      std::vector<TipsetBundle> chain,
      bool store_messages,
      const OnBlockStored &callback);

}  // namespace fc::sync::blocksync

#endif  // CPP_FILECOIN_SYNC_BLOCKSYNC_COMMON_HPP
