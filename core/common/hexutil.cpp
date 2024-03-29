/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "common/hexutil.hpp"

#include <boost/algorithm/hex.hpp>
#include <boost/format.hpp>
#include <gsl/span>

OUTCOME_CPP_DEFINE_CATEGORY(fc::common, UnhexError, e) {
  using fc::common::UnhexError;
  switch (e) {
    case UnhexError::kNonHexInput:
      return "Input contains non-hex characters";
    case UnhexError::kNotEnoughInput:
      return "Input contains odd number of characters";
    default:
      return "Unknown error";
  }
}

namespace fc::common {

  std::string hex_upper(const gsl::span<const uint8_t> bytes) noexcept {
    std::string res(bytes.size() * 2, '\x00');
    boost::algorithm::hex(bytes.begin(), bytes.end(), res.begin());
    return res;
  }

  std::string hex_lower(const gsl::span<const uint8_t> bytes) noexcept {
    std::string res(bytes.size() * 2, '\x00');
    boost::algorithm::hex_lower(bytes.begin(), bytes.end(), res.begin());
    return res;
  }

  outcome::result<std::vector<uint8_t>> unhex(std::string_view hex) {
    std::vector<uint8_t> blob;
    blob.reserve((hex.size() + 1) / 2);

    try {
      boost::algorithm::unhex(hex.begin(), hex.end(), std::back_inserter(blob));
      return blob;

    } catch (const boost::algorithm::not_enough_input &e) {
      return UnhexError::kNotEnoughInput;

    } catch (const boost::algorithm::non_hex_input &e) {
      return UnhexError::kNonHexInput;

    } catch (const std::exception &e) {
      return UnhexError::kUnknown;
    }
  }
}  // namespace fc::common
