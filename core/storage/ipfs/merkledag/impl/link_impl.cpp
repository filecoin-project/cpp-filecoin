/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "storage/ipfs/merkledag/impl/link_impl.hpp"

namespace fc::storage::ipfs::merkledag {
  LinkImpl::LinkImpl(libp2p::multi::ContentIdentifier id,
                     std::string name,
                     size_t size)
      : cid_{std::move(id)},
        name_{std::move(name)},
        size_{size} {}

  const std::string &LinkImpl::getName() const {
    return name_;
  }

  const CID &LinkImpl::getCID() const {
    return cid_;
  }

  size_t LinkImpl::getSize() const {
    return size_;
  }
}  // namespace fc::storage::ipfs::merkledag
