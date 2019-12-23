/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CID_GENERATOR_HPP
#define CPP_FILECOIN_CID_GENERATOR_HPP

#include <libp2p/crypto/random_generator/boost_generator.hpp>
#include <libp2p/multi/content_identifier.hpp>

namespace testutils {

  using libp2p::crypto::random::BoostRandomGenerator;
  using libp2p::crypto::random::CSPRNG;
  using libp2p::multi::ContentIdentifier;
  using libp2p::multi::HashType;
  using libp2p::multi::MulticodecType;
  using libp2p::multi::Multihash;

  /**
   * @brief Generate random Content Identifier (CID)
   */
  class CidGenerator {
   public:
    /**
     * Make random Content Identifier
     * @return CID
     */
    ContentIdentifier makeRandomCid() {
      auto version = ContentIdentifier::Version::V1;
      auto codec_type = MulticodecType::SHA2_256;
      auto size = 32u;  // sha256 bytes count
      auto hash =
          Multihash::create(HashType::sha256, generator->randomBytes(size));
      if (!hash) boost::throw_exception(std::system_error(hash.error()));

      return ContentIdentifier(version, codec_type, std::move(hash.value()));
    }

   private:
    std::shared_ptr<CSPRNG> generator =
        std::make_shared<BoostRandomGenerator>();
  };

}  // namespace testutils

#endif  // CPP_FILECOIN_CID_GENERATOR_HPP
