/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "filecoin/storage/ipld/impl/ipld_link_impl.hpp"

namespace fc::storage::ipld {
  IPLDLinkImpl::IPLDLinkImpl(libp2p::multi::ContentIdentifier id,
                             std::string name,
                             size_t size)
      : cid_{std::move(id)}, name_{std::move(name)}, size_{size} {}

  const std::string &IPLDLinkImpl::getName() const {
    return name_;
  }

  const CID &IPLDLinkImpl::getCID() const {
    return cid_;
  }

  size_t IPLDLinkImpl::getSize() const {
    return size_;
  }
}  // namespace fc::storage::ipld
