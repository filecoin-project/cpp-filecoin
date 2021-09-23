/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "cbor_blake/ipld_cbor.hpp"

namespace fc::adt {
  template <typename T>
  struct CbCidT {
    // TODO(turuslan): refactor CID to CbCid
    CID cid;
    // TODO(turuslan): refactor Ipld to CbIpld
    IpldPtr ipld{};

    CbCidT() = default;
    // TODO (a.chernyshov) (FIL-412) make constructor explicit
    // NOLINTNEXTLINE(google-explicit-constructor)
    CbCidT(CID cid) : cid{std::move(cid)} {}

    auto get() const {
      return getCbor<T>(ipld, cid);
    }
    outcome::result<void> set(const T &value) {
      OUTCOME_TRYA(cid, setCbor(ipld, value));
      return outcome::success();
    }

    // TODO (a.chernyshov) (FIL-412) make constructor explicit
    // NOLINTNEXTLINE(google-explicit-constructor)
    operator const CID &() const {
      return cid;
    }
  };
  template <typename T>
  CBOR2_DECODE(CbCidT<T>) {
    return s >> v.cid;
  }
  template <typename T>
  CBOR2_ENCODE(CbCidT<T>) {
    return s << v.cid;
  }
}  // namespace fc::adt

namespace fc::cbor_blake {
  template <typename T>
  struct CbLoadT<adt::CbCidT<T>> {
    static void call(CbIpldPtrIn ipld, adt::CbCidT<T> &cid) {
      cid.ipld = ipld;
    }
  };
}  // namespace fc::cbor_blake
