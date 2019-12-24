/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_TEST_TESTUTIL_CRYPTO_ENTROPY_HPP
#define CPP_FILECOIN_TEST_TESTUTIL_CRYPTO_ENTROPY_HPP

#include <gsl/span>

namespace fc::crypto {
  /**
   * Taken from
   * https://github.com/hyperledger/iroha-ed25519/blob/master/test/randombytes/random.cpp#L6
   * @brief calculates entropy of byte sequence
   * @param sequence  source byte sequence
   * @return entropy value
   */
  inline double entropy(gsl::span<uint8_t> sequence) {
    std::vector<uint8_t> freqs(256, 0);

    // calculate frequencies
    for (unsigned char i : sequence) {
      ++freqs[i];
    }

    double e = 0;
    for (const auto c : freqs) {
      double freq = (double)c / sequence.size();

      if (freq == 0) {
        continue;
      }

      e += freq * std::log2(freq);
    }

    return -e;
  }

  /**
   * @brief returns max possible entropy for a buffer of given size
   * @param volume is volume of alphabet
   * @return max value of entropy for given source alphabet volume
   */
  inline double max_entropy(size_t volume) {
    return log2(volume);
  }
}  // namespace fc::crypto

#endif  // CPP_FILECOIN_TEST_TESTUTIL_CRYPTO_ENTROPY_HPP
