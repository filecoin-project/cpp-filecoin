/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FILECOIN_STORAGE_IPLD_BLOCK_IMPL_HPP
#define FILECOIN_STORAGE_IPLD_BLOCK_IMPL_HPP

#include <map>
#include <vector>

#include <boost/optional.hpp>
#include <libp2p/multi/hash_type.hpp>
#include <libp2p/multi/multicodec_type.hpp>
#include "common/buffer.hpp"
#include "primitives/cid/cid.hpp"
#include "storage/ipld/ipld_block.hpp"

namespace fc::storage::ipld {
  using libp2p::multi::HashType;
  using ContentType = libp2p::multi::MulticodecType::Code;

  /**
   * @class IPLD data structure implementation
   */
  class IPLDBlockImpl : public virtual IPLDBlock {
   public:
    /**
     * @brief Construct IPLD block
     * @param version - CID version
     * @param hash_type - CID hash type
     * @param content_type - block content type
     */
    IPLDBlockImpl(CID::Version version,
                  HashType hash_type,
                  ContentType content_type);

    const CID &getCID() const override;

    const common::Buffer &getRawBytes() const override;

   protected:
    /**
     * @brief Clear cached CID and serialized values
     */
    void clearCache() const;

   private:
    CID::Version cid_version_;
    HashType cid_hash_type_;
    ContentType content_type_;

    mutable boost::optional<CID> cid_;
    mutable boost::optional<common::Buffer> raw_bytes_;
  };
}  // namespace fc::storage::ipld

#endif
