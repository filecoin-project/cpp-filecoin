/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "codec/cbor/cbor_codec.hpp"
#include "markets/retrieval/protocols/retrieval_protocol.hpp"
#include "primitives/address/address_codec.hpp"
#include "storage/ipfs/graphsync/extension.hpp"

namespace fc::markets::retrieval {
  CBOR2_ENCODE(DealProposalParams::Named) {
    auto m{s.map()};
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
    s.named(m, "Selector") >> v.selector;
    s.named(m, "PieceCID") >> v.piece;
    s.named(m, "PricePerByte") >> v.price_per_byte;
    s.named(m, "PaymentInterval") >> v.payment_interval;
    s.named(m, "PaymentIntervalIncrease") >> v.payment_interval_increase;
    s.named(m, "UnsealPrice") >> v.unseal_price;
    return s;
  }

  CBOR2_ENCODE(DealProposal::Named) {
    auto m{s.map()};
    m["PayloadCID"] << v.payload_cid;
    m["ID"] << v.deal_id;
    m["Params"] << (const DealProposalParams::Named &)v.params;
    return s << m;
  }
  CBOR2_DECODE(DealProposal::Named) {
    auto m{s.map()};
    s.named(m, "PayloadCID") >> v.payload_cid;
    s.named(m, "ID") >> v.deal_id;
    s.named(m, "Params") >> *(DealProposalParams::Named *)&v.params;
    return s;
  }

  CBOR2_ENCODE(DealResponse::Named) {
    auto m{s.map()};
    m["Status"] << v.status;
    m["ID"] << v.deal_id;
    m["PaymentOwed"] << v.payment_owed;
    m["Message"] << v.message;
    return s << m;
  }
  CBOR2_DECODE(DealResponse::Named) {
    auto m{s.map()};
    s.named(m, "Status") >> v.status;
    s.named(m, "ID") >> v.deal_id;
    s.named(m, "PaymentOwed") >> v.payment_owed;
    s.named(m, "Message") >> v.message;
    return s;
  }

  CBOR2_ENCODE(DealPayment::Named) {
    auto m{s.map()};
    m["ID"] << v.deal_id;
    m["PaymentChannel"] << v.payment_channel;
    m["PaymentVoucher"] << v.payment_voucher;
    return s << m;
  }
  CBOR2_DECODE(DealPayment::Named) {
    auto m{s.map()};
    s.named(m, "ID") >> v.deal_id;
    s.named(m, "PaymentChannel") >> v.payment_channel;
    s.named(m, "PaymentVoucher") >> v.payment_voucher;
    return s;
  }
}  // namespace fc::markets::retrieval

namespace fc::storage::ipfs::graphsync {
  CBOR2_DECODE(ResMeta) {
    auto m{s.map()};
    s.named(m, "link") >> v.cid;
    s.named(m, "blockPresent") >> v.present;
    return s;
  }
  CBOR2_ENCODE(ResMeta) {
    auto m{s.map()};
    m["link"] << v.cid;
    m["blockPresent"] << v.present;
    return s << m;
  }
}  // namespace fc::storage::ipfs::graphsync
