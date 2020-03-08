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
#include "filecoin/common/buffer.hpp"
#include "filecoin/crypto/hasher/hasher.hpp"
#include "filecoin/primitives/cid/cid.hpp"
#include "filecoin/storage/ipld/ipld_block.hpp"

namespace fc::storage::ipld {

  using HashType = libp2p::multi::HashType;
  using ContentType = libp2p::multi::MulticodecType::Code;

  /**
   * @class IPLD data structure implementation
   */
  template <CID::Version cid_version,
            HashType hash_type,
            ContentType content_type>
  class IPLDBlockCommon : public virtual IPLDBlock {
   public:
    const CID &getCID() const override {
      if (!cid_) {
        auto multi_hash = crypto::Hasher::calculate(hash_type, getRawBytes());
        CID id{cid_version, content_type, std::move(multi_hash)};
        cid_ = std::move(id);
      }
      return cid_.value();
    }

    const common::Buffer &getRawBytes() const override {
      if (!raw_bytes_) {
        auto bytes = getBlockContent();
        BOOST_ASSERT(bytes.has_value());
        raw_bytes_ = common::Buffer{std::move(bytes.value())};
      }
      return raw_bytes_.value();
    }

   protected:
    /**
     * @brief Clear cached CID and serialized values
     */
    void clearCache() const {
      cid_ = boost::none;
      raw_bytes_ = boost::none;
    }

    /**
     * @brief Get block content
     * @return IPLD block raw bytes
     */
    virtual outcome::result<std::vector<uint8_t>> getBlockContent() const = 0;

   private:
    mutable boost::optional<CID> cid_;
    mutable boost::optional<common::Buffer> raw_bytes_;
  };
}  // namespace fc::storage::ipld

#endif
