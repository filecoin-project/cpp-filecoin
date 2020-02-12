/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_STORAGE_IPFS_DATASTORE_KEY_HPP
#define CPP_FILECOIN_CORE_STORAGE_IPFS_DATASTORE_KEY_HPP

#include <string>

#include "common/outcome.hpp"

namespace fc::storage {
  /**
   * @struct Key datastore key
   */
  struct DatastoreKey {
    std::string value;

    /**
     * @brief creates key from string
     * @param value key data
     */
    static DatastoreKey makeFromString(std::string_view value) noexcept;

    /**
     * @brief creates raw key from string without safety checking
     * @param value key data
     */
    static outcome::result<DatastoreKey> makeRaw(
        std::string_view value) noexcept;
  };

  enum class DatastoreKeyError {
    INVALID_DATASTORE_KEY = 1,  /// invalid data used for creating datastore key
  };

  /** @brief cbor-encodes DatastoreKey instance */
  template <class Stream,
            typename = std::enable_if_t<
                std::remove_reference<Stream>::type::is_cbor_encoder_stream>>
  Stream &operator<<(Stream &&s, const DatastoreKey &key) {
    return s << (s.list() << key.value);
  }

  /** @brief cbor-decodes DatastoreKey instance */
  template <class Stream,
            typename = std::enable_if_t<
                std::remove_reference<Stream>::type::is_cbor_decoder_stream>>
  Stream &operator>>(Stream &&s, DatastoreKey &key) {
    s.list() >> key.value;
    return s;
  }

  /** @brief orders 2 DatastoreKey instances */
  inline bool operator<(const DatastoreKey &lhs, const DatastoreKey &rhs);

  /** @brief compares 2 DatastoreKey intances for equality */
  bool operator==(const DatastoreKey &lhs, const DatastoreKey &rhs);

}  // namespace fc::storage

OUTCOME_HPP_DECLARE_ERROR(fc::storage, DatastoreKeyError);

#endif  // CPP_FILECOIN_CORE_STORAGE_IPFS_DATASTORE_KEY_HPP
