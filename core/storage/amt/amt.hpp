/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/variant.hpp>

#include "codec/cbor/cbor_codec.hpp"
#include "common/outcome.hpp"
#include "common/visitor.hpp"
#include "common/which.hpp"
#include "primitives/cid/cid.hpp"
#include "storage/ipfs/datastore.hpp"

namespace fc::storage::amt {
  enum class AmtError {
    kExpectedCID = 1,
    kDecodeWrong,
    kIndexTooBig,
    kNotFound,
    kRootBitsWrong,
    kNodeBitsWrong,
  };
}  // namespace fc::storage::amt

OUTCOME_HPP_DECLARE_ERROR(fc::storage::amt, AmtError);

namespace fc::storage::amt {
  constexpr size_t kDefaultBits = 3;
  constexpr uint64_t kMaxIndex = UINT64_MAX - 1;

  using common::which;
  using Value = ipfs::IpfsDatastore::Value;

  struct Node {
    using Ptr = std::shared_ptr<Node>;
    using Link = boost::variant<CID, Ptr>;
    using Links = std::map<size_t, Link>;
    using Values = std::map<size_t, Value>;
    using Items = boost::variant<Values, Links>;

    Items items;
    size_t bits_bytes{};
  };
  CBOR2_DECODE_ENCODE(Node)

  using OptBitWidth = boost::optional<uint64_t>;
  struct Root {
    OptBitWidth bits;
    uint64_t height{};
    uint64_t count{};
    Node node;
  };
  CBOR2_DECODE_ENCODE(Root)

  class Amt {
   public:
    using Visitor =
        std::function<outcome::result<void>(uint64_t, const Value &)>;

    explicit Amt(std::shared_ptr<ipfs::IpfsDatastore> store,
                 OptBitWidth bit_width = {});
    Amt(std::shared_ptr<ipfs::IpfsDatastore> store,
        const CID &root,
        OptBitWidth bit_width = {});
    /// Get values quantity
    outcome::result<uint64_t> count() const;
    /// Set value by key, does not write to storage
    outcome::result<void> set(uint64_t key, gsl::span<const uint8_t> value);
    /// Get value by key
    outcome::result<Value> get(uint64_t key) const;
    /// Remove value by key, does not write to storage
    outcome::result<void> remove(uint64_t key);
    /// Checks if key is present
    outcome::result<bool> contains(uint64_t key) const;
    /// Write changes made by set and remove to storage
    outcome::result<CID> flush();
    /// Get root CID if flushed, throw otherwise
    const CID &cid() const;
    /// Apply visitor for key value pairs
    outcome::result<void> visit(const Visitor &visitor) const;
    /// Loads root item
    outcome::result<void> loadRoot() const;
    /// Store CBOR encoded value by key
    template <typename T>
    outcome::result<void> setCbor(uint64_t key, const T &value) {
      OUTCOME_TRY(bytes, cbor_blake::cbEncodeT(value));
      return set(key, bytes);
    }

    /// Get CBOR decoded value by key
    template <typename T>
    outcome::result<T> getCbor(uint64_t key) const {
      OUTCOME_TRY(bytes, get(key));
      return cbor_blake::cbDecodeT<T>(ipld, bytes);
    }

    IpldPtr ipld;

   private:
    outcome::result<bool> set(Node &node,
                              uint64_t height,
                              uint64_t key,
                              gsl::span<const uint8_t> value);
    outcome::result<bool> remove(Node &node, uint64_t height, uint64_t key);
    outcome::result<void> flush(Node &node);
    outcome::result<void> visit(Node &node,
                                uint64_t height,
                                uint64_t offset,
                                const Visitor &visitor) const;
    outcome::result<Node::Ptr> loadLink(Node &node,
                                        uint64_t index,
                                        bool create) const;
    uint64_t bits() const;
    uint64_t bitsBytes() const;
    uint64_t maskAt(uint64_t height) const;
    uint64_t maxAt(uint64_t height) const;

    mutable boost::variant<CID, Root> root_;
    OptBitWidth bits_;
  };
}  // namespace fc::storage::amt
