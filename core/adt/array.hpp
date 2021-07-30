/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "storage/amt/amt.hpp"

namespace fc::adt {
  /// Strongly typed amt wrapper
  template <typename Value, size_t bits = storage::amt::kDefaultBits>
  struct Array {
    using Key = uint64_t;
    using Visitor = std::function<outcome::result<void>(Key, const Value &)>;

    Array(IpldPtr ipld = nullptr) : amt{ipld, bits} {}

    Array(const CID &root, IpldPtr ipld = nullptr) : amt{ipld, root, bits} {}

    outcome::result<boost::optional<Value>> tryGet(Key key) const {
      auto maybe = get(key);
      if (!maybe) {
        if (maybe.error() != storage::amt::AmtError::kNotFound) {
          return maybe.error();
        }
        return boost::none;
      }
      return maybe.value();
    }

    outcome::result<bool> has(Key key) const {
      return amt.contains(key);
    }

    outcome::result<Value> get(Key key) const {
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

    outcome::result<uint64_t> size() const {
      OUTCOME_TRY(size, amt.count());
      return size;
    }

    outcome::result<void> visit(const Visitor &visitor) const {
      return amt.visit([&](auto key, auto &value) -> outcome::result<void> {
        OUTCOME_TRY(value2, cbor_blake::cbDecodeT<Value>(amt.ipld, value));
        return visitor(key, value2);
      });
    }

    outcome::result<std::vector<Value>> values() const {
      std::vector<Value> values;
      OUTCOME_TRY(visit([&](auto, auto &value) {
        values.push_back(value);
        return outcome::success();
      }));
      return values;
    }

    storage::amt::Amt amt;
  };

  /// Cbor encode array
  template <class Stream,
            typename Value,
            size_t bits,
            typename = std::enable_if_t<
                std::remove_reference_t<Stream>::is_cbor_encoder_stream>>
  Stream &operator<<(Stream &&s, const Array<Value, bits> &array) {
    return s << array.amt.cid();
  }

  /// Cbor decode array
  template <class Stream,
            typename Value,
            size_t bits,
            typename = std::enable_if_t<
                std::remove_reference_t<Stream>::is_cbor_decoder_stream>>
  Stream &operator>>(Stream &&s, Array<Value, bits> &array) {
    CID root;
    s >> root;
    array.amt = {nullptr, root, bits};
    return s;
  }
}  // namespace fc::adt

namespace fc::cbor_blake {
  template <typename V, size_t bits>
  struct CbLoadT<adt::Array<V, bits>> {
    static void call(CbIpldPtrIn ipld, adt::Array<V, bits> &array) {
      array.amt.ipld = ipld;
    }
  };

  template <typename V, size_t bits>
  struct CbFlushT<adt::Array<V, bits>> {
    static auto call(adt::Array<V, bits> &array) {
      return array.amt.flush();
    }
  };
}  // namespace fc::cbor_blake
