/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FILECOIN_STORAGE_IPLD_LINK_IMPL_HPP
#define FILECOIN_STORAGE_IPLD_LINK_IMPL_HPP

#include <string>
#include <utility>
#include <vector>

#include "filecoin/storage/ipld/ipld_link.hpp"

namespace fc::storage::ipld {
  class IPLDLinkImpl : public IPLDLink {
   public:
    /**
     * @brief Construct Link
     * @param id - CID of the target object
     * @param name - name of the target object
     * @param size - total size of the target object
     */
    IPLDLinkImpl(libp2p::multi::ContentIdentifier id,
                 std::string name,
                 size_t size);

    IPLDLinkImpl() = default;

    const std::string &getName() const override;

    const CID &getCID() const override;

    size_t getSize() const override;

   private:
    CID cid_;
    std::string name_;
    size_t size_{};
  };
}  // namespace fc::storage::ipld

#endif
