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

  /// Strongly typed amt wrapper
  template <typename Value>
  struct Array {
    using Key = uint64_t;
    using Visitor = std::function<outcome::result<void>(Key, const Value &)>;

    Array() : amt{nullptr} {}

    explicit Array(const CID &root) : amt{nullptr, root} {}

    void load(std::shared_ptr<Ipld> ipld) {
      amt.setIpld(ipld);
    }

    outcome::result<boost::optional<Value>> tryGet(Key key) {
      auto maybe = get(key);
      if (!maybe) {
        if (maybe.error() != storage::amt::AmtError::NOT_FOUND) {
          return maybe.error();
        }
        return boost::none;
      }
      return maybe.value();
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

    outcome::result<void> append(const Value &value) {
      OUTCOME_TRY(count, amt.count());
      return set(count, value);
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

  /// Cbor encode array
  template <class Stream,
            typename Value,
            typename = std::enable_if_t<
                std::remove_reference_t<Stream>::is_cbor_encoder_stream>>
  Stream &operator<<(Stream &&s, const Array<Value> &array) {
    return s << array.amt.cid();
  }

  /// Cbor decode array
  template <class Stream,
            typename Value,
            typename = std::enable_if_t<
                std::remove_reference_t<Stream>::is_cbor_decoder_stream>>
  Stream &operator>>(Stream &&s, Array<Value> &array) {
    CID root;
    s >> root;
    array.amt = {nullptr, root};
    return s;
  }
}  // namespace fc::adt

#endif  // CPP_FILECOIN_ARRAY_HPP
