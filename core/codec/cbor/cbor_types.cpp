/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "codec/cbor/cbor_codec.hpp"
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
