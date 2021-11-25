/**
* Copyright Soramitsu Co., Ltd. All Rights Reserved.
* SPDX-License-Identifier: Apache-2.0
*/

#include "markets/storage/ask_protocol.hpp"
#include "codec/cbor/cbor_codec.hpp"

namespace fc::markets::storage {
  using codec::cbor::CborDecodeStream;
  using codec::cbor::CborEncodeStream;

  CBOR2_ENCODE(StorageAsk::Named) {
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

  CBOR2_DECODE(StorageAsk::Named) {
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

  CBOR2_ENCODE(SignedStorageAsk::Named) {
    auto m{CborEncodeStream::map()};
    m["Ask"] << static_cast<const StorageAsk::Named &>(v.ask);
    m["Signature"] << v.signature;
    return s << m;
  }

  CBOR2_DECODE(SignedStorageAsk::Named) {
    auto m{s.map()};
    CborDecodeStream::named(m, "Ask")
        >> *(static_cast<StorageAsk::Named *>(&v.ask));
    CborDecodeStream::named(m, "Signature") >> v.signature;
    return s;
  }

  CBOR2_ENCODE(AskRequest::Named) {
    auto m{CborEncodeStream::map()};
    m["Miner"] << v.miner;
    return s << m;
  }

  CBOR2_DECODE(AskRequest::Named) {
    auto m{s.map()};

    CborDecodeStream::named(m, "Miner") >> v.miner;
    return s;
  }

  CBOR2_ENCODE(AskResponse::Named) {
    auto m{CborEncodeStream::map()};
    m["Ask"] << static_cast<const SignedStorageAsk::Named &>(v.ask);
    return s << m;
  }

  CBOR2_DECODE(AskResponse::Named) {
    auto m{s.map()};
    CborDecodeStream::named(m, "Ask")
        >> *(static_cast<SignedStorageAsk::Named *>(&v.ask));
    return s;
  }
}
