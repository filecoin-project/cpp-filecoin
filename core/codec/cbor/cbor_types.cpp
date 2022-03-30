/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "codec/cbor/cbor_codec.hpp"
#include "data_transfer/message.hpp"
#include "primitives/address/address_codec.hpp"
#include "storage/ipfs/graphsync/extension.hpp"

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

namespace fc::data_transfer {
  using codec::cbor::CborDecodeStream;
  using codec::cbor::CborEncodeStream;

  CBOR2_ENCODE(DataTransferRequest) {
    auto m{CborEncodeStream::map()};
    m["BCid"] << v.base_cid;
    m["Type"] << v.type;
    m["Paus"] << v.is_pause;
    m["Part"] << v.is_part;
    m["Pull"] << v.is_pull;
    m["Stor"] << v.selector;
    m["Vouch"] << v.voucher;
    m["VTyp"] << v.voucher_type;
    m["XferID"] << v.transfer_id;

    // note: https://github.com/filecoin-project/go-data-transfer/pull/315
    if (v.restart) {
      auto l{CborEncodeStream::list()};
      s << common::span::bytestr(v.restart->initiator.toVector());
      s << common::span::bytestr(v.restart->responder.toVector());
      l << v.restart->id;
      m["RestartChannel"] << l;
    } else {
      m["RestartChannel"] << (CborEncodeStream::list() << ""
                                                       << "" << 0);
    }

    return s << m;
  }
  CBOR2_DECODE(DataTransferRequest) {
    auto m{s.map()};
    CborDecodeStream::named(m, "BCid") >> v.base_cid;
    CborDecodeStream::named(m, "Type") >> v.type;
    CborDecodeStream::named(m, "Paus") >> v.is_pause;
    CborDecodeStream::named(m, "Part") >> v.is_part;
    CborDecodeStream::named(m, "Pull") >> v.is_pull;
    CborDecodeStream::named(m, "Stor") >> v.selector;
    CborDecodeStream::named(m, "Vouch") >> v.voucher;
    CborDecodeStream::named(m, "VTyp") >> v.voucher_type;
    CborDecodeStream::named(m, "XferID") >> v.transfer_id;

    // note: https://github.com/filecoin-project/go-data-transfer/pull/315
    {
      v.restart.reset();
      auto l{CborDecodeStream::named(m, "RestartChannel").list()};
      ChannelId restart;
      std::string initiator;
      std::string responder;
      TransferId id;
      l >> initiator >> responder >> id;
      if (!initiator.empty() && !responder.empty()) {
        v.restart = ChannelId{
            PeerId::fromBytes(common::span::cbytes(initiator)).value(),
            PeerId::fromBytes(common::span::cbytes(responder)).value(),
            id,
        };
      }
    }

    return s;
  }

  CBOR2_ENCODE(DataTransferResponse) {
    auto m{CborEncodeStream::map()};
    m["Type"] << v.type;
    m["Acpt"] << v.is_accepted;
    m["Paus"] << v.is_pause;
    m["XferID"] << v.transfer_id;
    m["VRes"] << v.voucher;
    m["VTyp"] << v.voucher_type;
    return s << m;
  }
  CBOR2_DECODE(DataTransferResponse) {
    auto m{s.map()};
    CborDecodeStream::named(m, "Type") >> v.type;
    CborDecodeStream::named(m, "Acpt") >> v.is_accepted;
    CborDecodeStream::named(m, "Paus") >> v.is_pause;
    CborDecodeStream::named(m, "XferID") >> v.transfer_id;
    CborDecodeStream::named(m, "VRes") >> v.voucher;
    CborDecodeStream::named(m, "VTyp") >> v.voucher_type;
    return s;
  }

  CBOR2_ENCODE(DataTransferMessage) {
    auto m{CborEncodeStream::map()};
    m["IsRq"] << v.is_request;
    m["Request"] << v.request;
    m["Response"] << v.response;
    return s << m;
  }
  CBOR2_DECODE(DataTransferMessage) {
    auto m{s.map()};
    CborDecodeStream::named(m, "IsRq") >> v.is_request;
    CborDecodeStream::named(m, "Request") >> v.request;
    CborDecodeStream::named(m, "Response") >> v.response;
    return s;
  }
}  // namespace fc::data_transfer
