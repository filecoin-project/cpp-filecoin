/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_COMMON_TAR_UTIL_HPP
#define CPP_FILECOIN_CORE_COMMON_TAR_UTIL_HPP

#include <string>
#include "common/outcome.hpp"

namespace fc::common {

  constexpr int kTarBlockSize = 10240;

  outcome::result<void> extractTar(const std::string &tar_path,
                                   const std::string &output_path);

  enum class TarErrors {
    kCannotCreateDir = 1,
    kCannotUntarArchive,
  };

}  // namespace fc::common

OUTCOME_HPP_DECLARE_ERROR(fc::common, TarErrors);

#endif  // CPP_FILECOIN_CORE_COMMON_TAR_UTIL_HPP
