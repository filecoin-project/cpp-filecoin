/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <string>

#include <libp2p/peer/protocol.hpp>
#include "codec/cbor/cbor_codec.hpp"
#include "codec/cbor/streams_annotation.hpp"
#include "primitives/address/address.hpp"
#include "primitives/address/address_codec.hpp"
#include "primitives/cid/cid.hpp"
#include "primitives/types.hpp"

namespace fc::markets::retrieval {
  using codec::cbor::CborDecodeStream;
  using codec::cbor::CborEncodeStream;
  using primitives::TokenAmount;
  using primitives::address::Address;

  /** Query Protocol ID v0.0.1 */
  const libp2p::peer::Protocol kQueryProtocolIdV0_0_1 =
      "/fil/retrieval/qry/0.0.1";

  /** Query Protocol ID v1.0.0 */
  const libp2p::peer::Protocol kQueryProtocolIdV1_0_0 =
      "/fil/retrieval/qry/1.0.0";

  /**
   * @struct Request from client to provider
   *         to retrieve specified data by CID
   */
  struct QueryParams {
    /* Identifier of the parent's Piece */
    boost::optional<CID> piece_cid;
  };

  struct QueryParamsV0_0_1 : public QueryParams {};

  CBOR_TUPLE(QueryParamsV0_0_1, piece_cid)

  struct QueryParamsV1_0_0 : public QueryParams {};

  inline CBOR2_ENCODE(QueryParamsV1_0_0) {
    auto m{CborEncodeStream::orderedMap()};
    m["PieceCID"] << v.piece_cid;
    return s << m;
  }

  inline CBOR2_DECODE(QueryParamsV1_0_0) {
    auto m{s.map()};
    CborDecodeStream::named(m, "PieceCID") >> v.piece_cid;
    return s;
  }

  CBOR_TUPLE(QueryParams, piece_cid)

  struct QueryRequest {
    /* Identifier of the requested item */
    CID payload_cid;

    /* Additional params */
    QueryParams params;
  };

  struct QueryRequestV0_0_1 : public QueryRequest {};

  CBOR_TUPLE(QueryRequestV0_0_1, payload_cid, params)

  struct QueryRequestV1_0_0 : public QueryRequest {};

  inline CBOR2_ENCODE(QueryRequestV1_0_0) {
    auto m{CborEncodeStream::orderedMap()};
    m["PayloadCID"] << v.payload_cid;
    m["QueryParams"] << QueryParamsV1_0_0{v.params};
    return s << m;
  }

  inline CBOR2_DECODE(QueryRequestV1_0_0) {
    auto m{s.map()};
    CborDecodeStream::named(m, "PayloadCID") >> v.payload_cid;
    v.params =
        CborDecodeStream::named(m, "QueryParams").get<QueryParamsV1_0_0>();
    return s;
  }

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
    QueryResponseStatus response_status{};

    /* Current item status */
    QueryItemStatus item_status{};

    /* Size of the requested piece in bytes */
    size_t item_size{};

    /* Address to send tokens, may be different than miner address */
    Address payment_address;

    /* Min token amount per byte */
    TokenAmount min_price_per_byte;

    /* Max number of bytes a provider will send before requesting next payment
     */
    uint64_t payment_interval{};

    /* Max rate at which previous value increases */
    uint64_t interval_increase{};

    /* Optional text message */
    std::string message;

    TokenAmount unseal_price;
  };

  struct QueryResponseV0_0_1 : public QueryResponse {};

  CBOR_TUPLE(QueryResponseV0_0_1,
             response_status,
             item_status,
             item_size,
             payment_address,
             min_price_per_byte,
             payment_interval,
             interval_increase,
             message,
             unseal_price)

  struct QueryResponseV1_0_0 : public QueryResponse {};

  inline CBOR2_ENCODE(QueryResponseV1_0_0) {
    auto m{CborEncodeStream::orderedMap()};
    m["Status"] << v.response_status;
    m["PieceCIDFound"] << v.item_status;
    m["Size"] << v.item_size;
    m["PaymentAddress"] << v.payment_address;
    m["MinPricePerByte"] << v.min_price_per_byte;
    m["MaxPaymentInterval"] << v.payment_interval;
    m["MaxPaymentIntervalIncrease"] << v.interval_increase;
    m["Message"] << v.message;
    m["UnsealPrice"] << v.unseal_price;
    return s << m;
  }

  inline CBOR2_DECODE(QueryResponseV1_0_0) {
    auto m{s.map()};
    CborDecodeStream::named(m, "Status") >> v.response_status;
    CborDecodeStream::named(m, "PieceCIDFound") >> v.item_status;
    CborDecodeStream::named(m, "Size") >> v.item_size;
    CborDecodeStream::named(m, "PaymentAddress") >> v.payment_address;
    CborDecodeStream::named(m, "MinPricePerByte") >> v.min_price_per_byte;
    CborDecodeStream::named(m, "MaxPaymentInterval") >> v.payment_interval;
    CborDecodeStream::named(m, "MaxPaymentIntervalIncrease")
        >> v.interval_increase;
    CborDecodeStream::named(m, "Message") >> v.message;
    CborDecodeStream::named(m, "UnsealPrice") >> v.unseal_price;
    return s;
  }

}  // namespace fc::markets::retrieval
