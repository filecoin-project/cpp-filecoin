/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/ipld/impl/ipld_block_impl.hpp"

#include "crypto/hasher/hasher.hpp"

namespace fc::storage::ipld {

  IPLDBlockImpl::IPLDBlockImpl(CID::Version version,
                               HashType hash_type,
                               ContentType content_type)
      : cid_version_{version},
        cid_hash_type_{hash_type},
        content_type_{content_type} {}

  const CID &IPLDBlockImpl::getCID() const {
    if (!cid_) {
      auto multi_hash =
          crypto::Hasher::calculate(cid_hash_type_, getRawBytes());
      CID id{cid_version_, content_type_, std::move(multi_hash)};
      cid_ = std::move(id);
    }
    return cid_.value();
  }

  const common::Buffer &IPLDBlockImpl::getRawBytes() const {
    if (!raw_bytes_) {
      raw_bytes_ = serialize();
    }
    return raw_bytes_.value();
  }

  void IPLDBlockImpl::clearCache() const {
    cid_ = boost::none;
    raw_bytes_ = boost::none;
  }
}  // namespace fc::storage::ipld
