/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/filesystem/path.hpp>
#include <string>
#include "common/outcome.hpp"

namespace fc::common {

  constexpr int kTarBlockSize = 10240;

  outcome::result<void> zipTar(const boost::filesystem::path &input_path,
                               const boost::filesystem::path &output_path);

  outcome::result<void> extractTar(const boost::filesystem::path &tar_path,
                                   const boost::filesystem::path &output_path);

  enum class TarErrors {
    kCannotCreateDir = 1,
    kCannotUntarArchive,
    kCannotZipTarArchive,
    kCannotOpenFile,
    kCannotReadFile,
  };

}  // namespace fc::common

OUTCOME_HPP_DECLARE_ERROR(fc::common, TarErrors);
