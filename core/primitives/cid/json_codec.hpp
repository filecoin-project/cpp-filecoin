/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_PRIMITIVES_CID_JSON_CODEC_HPP
#define CPP_FILECOIN_CORE_PRIMITIVES_CID_JSON_CODEC_HPP

#include "common/outcome.hpp"
#include "primitives/cid/cid.hpp"
#include "common/buffer.hpp"

namespace fc::codec::json {
  enum class JsonCodecError : int { BAD_JSON = 1, WRONG_CID_ARRAY_FORMAT };
  /**
   * @brief json-encodes span of CID objects
   */
  outcome::result<std::string> encodeCidVector(gsl::span<const CID> span);

  /**
   * @brief tries to json-decode vector of CID objects
   * @param data source
   * @return vector of CID objects or error
   */
  outcome::result<std::vector<CID>> decodeCidVector(std::string_view data);
}  // namespace fc::codec::json

OUTCOME_HPP_DECLARE_ERROR(fc::codec::json, JsonCodecError);

#endif  // CPP_FILECOIN_CORE_PRIMITIVES_CID_JSON_CODEC_HPP
