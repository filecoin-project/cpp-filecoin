/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_TESTUTILS_BUFFER_GENERATOR_HPP
#define CPP_FILECOIN_TESTUTILS_BUFFER_GENERATOR_HPP

#include <libp2p/crypto/random_generator/boost_generator.hpp>

#include "common/buffer.hpp"

using fc::common::Buffer;
using libp2p::crypto::random::BoostRandomGenerator;
using libp2p::crypto::random::CSPRNG;

namespace testutils {

  /**
   * @brief Random buffer generator
   */
  class BufferGenerator {
   public:
    static constexpr size_t kDefaultBufferLength = 32;

    /**
     * Generate random buffer if size length
     * @param size - length of generated buffer
     * @return random bytes buffer
     */
    Buffer makeRandomBuffer(size_t size = kDefaultBufferLength) {
      return Buffer{generator->randomBytes(size)};
    }

   private:
    std::shared_ptr<CSPRNG> generator =
        std::make_shared<BoostRandomGenerator>();
  };

}  // namespace testutils

#endif  // CPP_FILECOIN_TESTUTILS_BUFFER_GENERATOR_HPP
