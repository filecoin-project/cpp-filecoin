/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

// TODO(turuslan): refactor Ipld to CbIpld
#include "storage/ipfs/datastore.hpp"

namespace fc::primitives::block {
  struct BlockHeader;
}  // namespace fc::primitives::block

namespace fc::cbor_blake {
  // TODO(turuslan): refactor Ipld to CbIpld
  using CbIpldPtr = IpldPtr;
  using CbIpldPtrIn = const CbIpldPtr &;

  template <typename T>
  void cbLoadT(CbIpldPtrIn ipld, T &value);
  template <typename T>
  outcome::result<void> cbFlushT(T &value);

  template <typename T>
  outcome::result<T> cbDecodeT(CbIpldPtrIn ipld, BytesIn cbor) {
    OUTCOME_TRY(value, codec::cbor::decode<T>(cbor));
    cbLoadT(ipld, value);
    return std::move(value);
  }

  template <typename T>
  outcome::result<Buffer> cbEncodeT(const T &value) {
    OUTCOME_TRY(cbFlushT(value));
    return codec::cbor::encode(value);
  }

  // TODO(turuslan): refactor CID to CbCid
  template <typename T>
  outcome::result<T> getCbor(CbIpldPtrIn ipld, const CID &key) {
    OUTCOME_TRY(cbor, ipld->get(key));
    return cbDecodeT<T>(ipld, cbor);
  }

  // TODO(turuslan): refactor CID to CbCid
  template <typename T>
  outcome::result<CID> setCbor(CbIpldPtrIn ipld, const T &value) {
    static_assert(!std::is_same_v<std::remove_cv_t<std::remove_reference_t<T>>,
                                  primitives::block::BlockHeader>);
    OUTCOME_TRY(cbor, cbEncodeT(value));
    CID key{CbCid::hash(cbor)};
    OUTCOME_TRY(ipld->set(key, std::move(cbor)));
    return key;
  }

  template <typename T>
  struct CbVisitT {
    template <typename Visitor>
    static void call(T &, const Visitor &) {}
  };

  template <typename T>
  struct CbLoadT {
    static void call(CbIpldPtrIn ipld, T &value) {
      CbVisitT<T>::call(value, [&](auto &x) { cbLoadT(ipld, x); });
    }
  };

  template <typename T>
  struct CbFlushT {
    static outcome::result<void> call(T &value) {
      outcome::result<void> result{outcome::success()};
      CbVisitT<T>::call(value, [&](auto &x) {
        if (result) {
          result = cbFlushT(x);
        }
      });
      return result;
    }
  };

  template <typename T>
  void cbLoadT(CbIpldPtrIn ipld, T &value) {
    CbLoadT<T>::call(ipld, value);
  }

  template <typename T>
  outcome::result<void> cbFlushT(T &value) {
    using Z = std::remove_const_t<T>;
    OUTCOME_TRY(CbFlushT<Z>::call(const_cast<Z &>(value)));
    return outcome::success();
  }
}  // namespace fc::cbor_blake

namespace fc {
  using cbor_blake::getCbor;
  using cbor_blake::setCbor;
}  // namespace fc
