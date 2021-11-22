/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "codec/cbor/cbor_codec.hpp"
#include "markets/retrieval/protocols/retrieval_protocol.hpp"
#include "markets/storage/ask_protocol.hpp"
#include "primitives/address/address_codec.hpp"
#include "storage/ipfs/graphsync/extension.hpp"

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
}  // namespace fc::markets::storage

namespace fc::markets::retrieval {
  using codec::cbor::CborDecodeStream;
  using codec::cbor::CborEncodeStream;

  CBOR2_ENCODE(DealProposalParams::Named) {
    auto m{CborEncodeStream::map()};
    m["Selector"] << v.selector;
    m["PieceCID"] << v.piece;
    m["PricePerByte"] << v.price_per_byte;
    m["PaymentInterval"] << v.payment_interval;
    m["PaymentIntervalIncrease"] << v.payment_interval_increase;
    m["UnsealPrice"] << v.unseal_price;
    return s << m;
  }
  CBOR2_DECODE(DealProposalParams::Named) {
    auto m{s.map()};
    CborDecodeStream::named(m, "Selector") >> v.selector;
    CborDecodeStream::named(m, "PieceCID") >> v.piece;
    CborDecodeStream::named(m, "PricePerByte") >> v.price_per_byte;
    CborDecodeStream::named(m, "PaymentInterval") >> v.payment_interval;
    CborDecodeStream::named(m, "PaymentIntervalIncrease")
        >> v.payment_interval_increase;
    CborDecodeStream::named(m, "UnsealPrice") >> v.unseal_price;
    return s;
  }

  CBOR2_ENCODE(DealProposal::Named) {
    auto m{CborEncodeStream::map()};
    m["PayloadCID"] << v.payload_cid;
    m["ID"] << v.deal_id;
    m["Params"] << static_cast<const DealProposalParams::Named &>(v.params);
    return s << m;
  }
  CBOR2_DECODE(DealProposal::Named) {
    auto m{s.map()};
    CborDecodeStream::named(m, "PayloadCID") >> v.payload_cid;
    CborDecodeStream::named(m, "ID") >> v.deal_id;
    CborDecodeStream::named(m, "Params")
        >> *(static_cast<DealProposalParams::Named *>(&v.params));
    return s;
  }

  CBOR2_ENCODE(DealResponse::Named) {
    auto m{CborEncodeStream::map()};
    m["Status"] << v.status;
    m["ID"] << v.deal_id;
    m["PaymentOwed"] << v.payment_owed;
    m["Message"] << v.message;
    return s << m;
  }
  CBOR2_DECODE(DealResponse::Named) {
    auto m{s.map()};
    CborDecodeStream::named(m, "Status") >> v.status;
    CborDecodeStream::named(m, "ID") >> v.deal_id;
    CborDecodeStream::named(m, "PaymentOwed") >> v.payment_owed;
    CborDecodeStream::named(m, "Message") >> v.message;
    return s;
  }

  CBOR2_ENCODE(DealPayment::Named) {
    auto m{CborEncodeStream::map()};
    m["ID"] << v.deal_id;
    m["PaymentChannel"] << v.payment_channel;
    m["PaymentVoucher"] << v.payment_voucher;
    return s << m;
  }
  CBOR2_DECODE(DealPayment::Named) {
    auto m{s.map()};
    CborDecodeStream::named(m, "ID") >> v.deal_id;
    CborDecodeStream::named(m, "PaymentChannel") >> v.payment_channel;
    CborDecodeStream::named(m, "PaymentVoucher") >> v.payment_voucher;
    return s;
  }
}  // namespace fc::markets::retrieval

namespace fc::storage::ipfs::graphsync {
  using codec::cbor::CborDecodeStream;
  using codec::cbor::CborEncodeStream;

  CBOR2_DECODE(ResMeta) {
    auto m{s.map()};
    CborDecodeStream::named(m, "link") >> v.cid;
    CborDecodeStream::named(m, "blockPresent") >> v.present;
    return s;
  }
  CBOR2_ENCODE(ResMeta) {
    auto m{CborEncodeStream::map()};
    m["link"] << v.cid;
    m["blockPresent"] << v.present;
    return s << m;
  }
}  // namespace fc::storage::ipfs::graphsync
