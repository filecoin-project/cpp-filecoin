/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_SYNC_BLOCKSYNC_COMMON_HPP
#define CPP_FILECOIN_SYNC_BLOCKSYNC_COMMON_HPP

#include "common.hpp"

#include "codec/cbor/cbor.hpp"

namespace fc::sync::blocksync {

  constexpr auto kProtocolId = "/fil/sync/blk/0.0.1";

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

    struct Messages {
      std::vector<UnsignedMessage> bls_msgs;
      MsgIncudes bls_msg_includes;
      std::vector<primitives::block::SignedMessage> secp_msgs;
      MsgIncudes secp_msg_includes;
    } messages;
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
  CBOR_TUPLE(TipsetBundle::Messages,
             bls_msgs,
             bls_msg_includes,
             secp_msgs,
             secp_msg_includes);
  CBOR_TUPLE(TipsetBundle, blocks, messages);
  CBOR_TUPLE(Response, status, message, chain);

}  // namespace fc::sync::blocksync

#endif  // CPP_FILECOIN_SYNC_BLOCKSYNC_COMMON_HPP
