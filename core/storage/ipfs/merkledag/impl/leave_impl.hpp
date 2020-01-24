/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FILECOIN_STORAGE_IPFS_MERKLEDAG_LEAVE_IMPL_HPP
#define FILECOIN_STORAGE_IPFS_MERKLEDAG_LEAVE_IMPL_HPP

#include "storage/ipfs/merkledag/leave.hpp"

#include <map>
#include <string>

namespace fc::storage::ipfs::merkledag {
  class LeaveImpl : public Leave {
   public:
    /**
     * @brief Construct leave
     * @param data - leave content
     */
    explicit LeaveImpl(common::Buffer data);

    const common::Buffer &content() const override;

    size_t count() const override;

    outcome::result<std::reference_wrapper<const Leave>> subLeave(
        std::string_view name) const override;

    std::vector<std::string_view> getSubLeaveNames() const override;

    /**
     * @brief Insert children leave
     * @param name - id of the leave
     * @param children - leave to insert
     * @return operation result
     */
    outcome::result<void> insertSubLeave(std::string name, LeaveImpl children);

   private:
    common::Buffer content_;
    std::map<std::string, LeaveImpl, std::less<>> children_;
  };
}  // namespace fc::storage::ipfs::merkledag

#endif
