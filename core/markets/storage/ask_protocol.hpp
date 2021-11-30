/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <libp2p/peer/protocol.hpp>
#include "codec/cbor/cbor_codec.hpp"
#include "codec/cbor/streams_annotation.hpp"
#include "crypto/signature/signature.hpp"
#include "primitives/address/address.hpp"
#include "primitives/address/address_codec.hpp"
#include "primitives/chain_epoch/chain_epoch.hpp"
#include "primitives/piece/piece.hpp"
#include "primitives/types.hpp"

namespace fc::markets::storage {
  using codec::cbor::CborDecodeStream;
  using codec::cbor::CborEncodeStream;
  using crypto::signature::Signature;
  using primitives::ChainEpoch;
  using primitives::TokenAmount;
  using primitives::address::Address;
  using primitives::piece::PaddedPieceSize;

  const libp2p::peer::Protocol kAskProtocolId_v1_0_1 = "/fil/storage/ask/1.0.1";

  /** Protocol 1.1.1 uses named cbor */
  const libp2p::peer::Protocol kAskProtocolId_v1_1_0 = "/fil/storage/ask/1.1.0";

  /**
   * StorageAsk defines the parameters by which a miner will choose to accept or
   * reject a deal.
   */
  struct StorageAsk {
    // Price per GiB / Epoch
    TokenAmount price;
    TokenAmount verified_price;
    PaddedPieceSize min_piece_size;
    PaddedPieceSize max_piece_size;
    Address miner;
    ChainEpoch timestamp{};
    ChainEpoch expiry{};
    uint64_t seq_no{};
  };

  /** StorageAsk used in V1.0.1 */
  struct StorageAskV1_0_1 : public StorageAsk {};

  CBOR_TUPLE(StorageAskV1_0_1,
             price,
             verified_price,
             min_piece_size,
             max_piece_size,
             miner,
             timestamp,
             expiry,
             seq_no)

  /** StorageAsk used in V1.1.0. Cbores with field names. */
  struct StorageAskV1_1_0 : public StorageAsk {};

  inline CBOR2_ENCODE(StorageAskV1_1_0) {
    auto m{CborEncodeStream::map()};
    m["Price"] << v.price;
    m["VerifiedPrice"] << v.verified_price;
    m["MinPieceSize"] << v.min_piece_size;
    m["MaxPieceSize"] << v.max_piece_size;
    m["Miner"] << v.miner;
    m["Timestamp"] << v.timestamp;
    m["Expiry"] << v.expiry;
    m["SeqNo"] << v.seq_no;
    return s << m;
  }

  inline CBOR2_DECODE(StorageAskV1_1_0) {
    auto m{s.map()};
    CborDecodeStream::named(m, "Price") >> v.price;
    CborDecodeStream::named(m, "VerifiedPrice") >> v.verified_price;
    CborDecodeStream::named(m, "MinPieceSize") >> v.min_piece_size;
    CborDecodeStream::named(m, "MaxPieceSize") >> v.max_piece_size;
    CborDecodeStream::named(m, "Miner") >> v.miner;
    CborDecodeStream::named(m, "Timestamp") >> v.timestamp;
    CborDecodeStream::named(m, "Expiry") >> v.expiry;
    CborDecodeStream::named(m, "SeqNo") >> v.seq_no;
    return s;
  }

  struct SignedStorageAsk {
    virtual ~SignedStorageAsk() = default;

    StorageAsk ask;
    Signature signature;

    /** Returns response digset */
    virtual outcome::result<Bytes> getDigest() const = 0;
  };

  CBOR_TUPLE(SignedStorageAsk, ask, signature)

  /** SignedStorageAsk used in V1.0.1 */
  struct SignedStorageAskV1_0_1 : public SignedStorageAsk {
    SignedStorageAskV1_0_1() = default;

    outcome::result<Bytes> getDigest() const override {
      return codec::cbor::encode(StorageAskV1_0_1{this->ask});
    };
  };

  inline CBOR2_ENCODE(SignedStorageAskV1_0_1) {
    return s << StorageAskV1_0_1{v.ask} << v.signature;
  }
  inline CBOR2_DECODE(SignedStorageAskV1_0_1) {
    StorageAskV1_0_1 ask;
    s >> ask;
    v.ask = ask;
    s >> v.signature;
    return s;
  }

  /** SignedStorageAsk used in V1.1.0 with named fields. */
  struct SignedStorageAskV1_1_0 : public SignedStorageAsk {
    SignedStorageAskV1_1_0() = default;
    explicit SignedStorageAskV1_1_0(StorageAsk ask) {
      this->ask = std::move(ask);
    }
    SignedStorageAskV1_1_0(StorageAsk ask, Signature signature) {
      this->ask = std::move(ask);
      this->signature = std::move(signature);
    }

    outcome::result<Bytes> getDigest() const override {
      return codec::cbor::encode(StorageAskV1_1_0{this->ask});
    };
  };

  inline CBOR2_ENCODE(SignedStorageAskV1_1_0) {
    auto m{CborEncodeStream::map()};
    m["Ask"] << StorageAskV1_1_0{v.ask};
    m["Signature"] << v.signature;
    return s << m;
  }

  inline CBOR2_DECODE(SignedStorageAskV1_1_0) {
    auto m{s.map()};
    StorageAskV1_1_0 ask;
    CborDecodeStream::named(m, "Ask") >> ask;
    v.ask = ask;
    CborDecodeStream::named(m, "Signature") >> v.signature;
    return s;
  }

  /**
   * AskRequest is a request for current ask parameters for a given miner
   */
  struct AskRequest {
    Address miner;
  };

  /** AskRequest used in V1.0.1 */
  struct AskRequestV1_0_1 : public AskRequest {};

  CBOR_TUPLE(AskRequestV1_0_1, miner)

  /** AskRequest used in V1.1.0. Cbores with field names. */
  struct AskRequestV1_1_0 : public AskRequest {};

  inline CBOR2_ENCODE(AskRequestV1_1_0) {
    auto m{CborEncodeStream::map()};
    m["Miner"] << v.miner;
    return s << m;
  }

  inline CBOR2_DECODE(AskRequestV1_1_0) {
    auto m{s.map()};
    CborDecodeStream::named(m, "Miner") >> v.miner;
    return s;
  }

  /**
   * AskResponse is the response sent over the network in response to an ask
   * request
   */
  struct AskResponse {
    virtual ~AskResponse() = default;

    virtual const SignedStorageAsk &ask() const = 0;
  };

  struct AskResponseV1_0_1 : public AskResponse {
    AskResponseV1_0_1() = default;

    explicit AskResponseV1_0_1(SignedStorageAskV1_0_1 ask)
        : ask_(std::move(ask)) {}

    const SignedStorageAsk &ask() const override {
      return ask_;
    }

   private:
    friend CborEncodeStream &operator<<(CborEncodeStream &,
                                        const AskResponseV1_0_1 &);
    friend CborDecodeStream &operator>>(CborDecodeStream &,
                                        AskResponseV1_0_1 &);

    SignedStorageAskV1_0_1 ask_;
  };

  inline CBOR2_ENCODE(AskResponseV1_0_1) {
    return s << v.ask_;
  }
  inline CBOR2_DECODE(AskResponseV1_0_1) {
    s >> v.ask_;
    return s;
  }

  struct AskResponseV1_1_0 : public AskResponse {
    AskResponseV1_1_0() = default;

    explicit AskResponseV1_1_0(SignedStorageAskV1_1_0 ask)
        : ask_(std::move(ask)) {}

    const SignedStorageAsk &ask() const override {
      return ask_;
    }

   private:
    friend CborEncodeStream &operator<<(CborEncodeStream &,
                                        const AskResponseV1_1_0 &);
    friend CborDecodeStream &operator>>(CborDecodeStream &,
                                        AskResponseV1_1_0 &);

    SignedStorageAskV1_1_0 ask_;
  };

  inline CBOR2_ENCODE(AskResponseV1_1_0) {
    auto m{CborEncodeStream::map()};
    m["Ask"] << v.ask_;
    return s << m;
  }

  inline CBOR2_DECODE(AskResponseV1_1_0) {
    auto m{s.map()};
    CborDecodeStream::named(m, "Ask") >> v.ask_;
    return s;
  }

}  // namespace fc::markets::storage
