/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "codec/cbor/cbor_codec.hpp"
#include "markets/retrieval/protocols/retrieval_protocol.hpp"
#include "primitives/address/address_codec.hpp"
#include "storage/ipfs/graphsync/extension.hpp"

namespace fc::markets::retrieval {
  using codec::cbor::CborDecodeStream;
  using codec::cbor::CborEncodeStream;

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
