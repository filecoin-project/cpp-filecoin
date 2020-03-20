/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_ARRAY_HPP
#define CPP_FILECOIN_ARRAY_HPP

#include "storage/amt/amt.hpp"

namespace fc::adt {
  using storage::amt::Amt;
  using Ipld = storage::ipfs::IpfsDatastore;

  template <typename Value>
  struct Array {
    using Key = uint64_t;
    using Visitor = std::function<outcome::result<void>(Key, const Value &)>;

    Array() : amt{nullptr} {}

    // NOLINTNEXTLINE(google-explicit-constructor)
    Array(const CID &root) : amt{nullptr, root} {}

    void load(std::shared_ptr<Ipld> ipld) {
      amt.setIpld(ipld);
    }

    outcome::result<bool> has(Key key) {
      return amt.contains(key);
    }

    outcome::result<Value> get(Key key) {
      return amt.getCbor<Value>(key);
    }

    outcome::result<void> set(Key key, const Value &value) {
      return amt.setCbor(key, value);
    }

    outcome::result<void> remove(Key key) {
      return amt.remove(key);
    }

    outcome::result<void> flush() {
      OUTCOME_TRY(amt.flush());
      return outcome::success();
    }

    outcome::result<void> visit(const Visitor &visitor) {
      return amt.visit([&](auto key, auto &value) -> outcome::result<void> {
        OUTCOME_TRY(value2, codec::cbor::decode<Value>(value));
        return visitor(key, value2);
      });
    }

    storage::amt::Amt amt;
  };

  template <class Stream,
            typename V,
            typename = std::enable_if_t<
                std::remove_reference_t<Stream>::is_cbor_encoder_stream>>
  Stream &operator<<(Stream &&s, const Array<V> &array) {
    return s << array.amt.cid();
  }

  template <class Stream,
            typename V,
            typename = std::enable_if_t<
                std::remove_reference_t<Stream>::is_cbor_decoder_stream>>
  Stream &operator>>(Stream &&s, Array<V> &array) {
    CID root;
    s >> root;
    array.amt = {nullptr, root};
    return s;
  }
}  // namespace fc::adt

#endif  // CPP_FILECOIN_ARRAY_HPP
