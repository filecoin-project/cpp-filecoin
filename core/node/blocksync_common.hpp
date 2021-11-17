/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "codec/cbor/cbor_codec.hpp"
#include "node/common.hpp"

namespace fc::sync::blocksync {

  constexpr auto kProtocolId = "/fil/sync/blk/0.0.1";

  enum RequestOptions {
    kBlocksOnly = 1,
    kMessagesOnly = 2,
    kBlocksAndMessages = 3,
  };

  struct Request {
    std::vector<CbCid> block_cids;
    uint64_t depth = 1;
    RequestOptions options = kBlocksAndMessages;
  };

  using MsgIncudes = std::vector<std::vector<uint64_t>>;

  struct TipsetBundle {
    // TODO(turuslan): use not so heap consuming containers, like small_vector

    std::vector<BlockHeader> blocks;

    struct Messages {
      std::vector<UnsignedMessage> bls_msgs;
      MsgIncudes bls_msg_includes;
      std::vector<primitives::block::SignedMessage> secp_msgs;
      MsgIncudes secp_msg_includes;
    };
    boost::optional<Messages> messages;
  };

  enum class ResponseStatus : int {
    kResponseComplete = 0,
    kResponsePartial = 101,
    kBlockNotFound = 201,
    kGoAway = 202,
    kInternalError = 203,
    kBadRequest = 204,
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
