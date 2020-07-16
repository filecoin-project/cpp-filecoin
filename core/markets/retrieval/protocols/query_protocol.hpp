/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_MARKETS_RETRIEVAL_QUERY_PROTOCOL_HPP
#define CPP_FILECOIN_CORE_MARKETS_RETRIEVAL_QUERY_PROTOCOL_HPP

#include <string>

#include <libp2p/peer/protocol.hpp>
#include "codec/cbor/streams_annotation.hpp"
#include "primitives/address/address.hpp"
#include "primitives/address/address_codec.hpp"
#include "primitives/cid/cid.hpp"
#include "primitives/types.hpp"

namespace fc::markets::retrieval {
  using primitives::TokenAmount;
  using primitives::address::Address;

  /**
   * @brief Query Protocol ID V0
   */
  const libp2p::peer::Protocol kQueryProtocolId = "/fil/retrieval/qry/0.0.1";

  /**
   * @struct Request from client to provider
   *         to retrieve specified data by CID
   */
  struct QueryParams {
    /* Identifier of the parent's Piece */
    boost::optional<CID> piece_cid;
  };

  struct QueryRequest {
    /* V0 protocol */
    /* Identifier of the requested item */
    CID payload_cid;

    /* V1 protocol */
    /* Additional params */
    QueryParams params;
  };

  CBOR_TUPLE(QueryParams, piece_cid)

  CBOR_TUPLE(QueryRequest, payload_cid, params)

  /**
   * @enum Status of the query response
   */
  enum class QueryResponseStatus : uint64_t {
    /* Provider has a piece and is prepared to return it */
    kQueryResponseAvailable,

    /* Provider either does not have a piece or cannot serve request */
    kQueryResponseUnavailable,

    /* Something went wrong generating a query response */
    kQueryResponseError
  };

  /**
   * @enum Status of the queried item
   */
  enum class QueryItemStatus : uint64_t {
    /* Requested part of the piece is available to be served */
    kQueryItemAvailable,

    /* Requested part of the piece is unavailable or cannot be served */
    kQueryItemUnavailable,

    /* Cannot determine if the given item is part of the requested piece */
    kQueryItemUnknown
  };

  /**
   * @struct Response from provider with initial retrieval params
   */
  struct QueryResponse {
    /* Current response status */
    QueryResponseStatus response_status;

    /* Current item status */
    QueryItemStatus item_status;

    /* Size of the requested piece in bytes */
    size_t item_size;

    /* Address to send tokens, may be different than miner address */
    Address payment_address;

    /* Min token amount per byte */
    TokenAmount min_price_per_byte;

    /* Max number of bytes a provider will send before requesting next payment
     */
    uint64_t payment_interval;

    /* Max rate at which previous value increases */
    uint64_t interval_increase;

    /* Optional text message */
    std::string message;
  };

  CBOR_TUPLE(QueryResponse,
             response_status,
             item_status,
             item_size,
             payment_address,
             min_price_per_byte,
             payment_interval,
             interval_increase,
             message);

}  // namespace fc::markets::retrieval

#endif
