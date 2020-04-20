/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/piece/piece_data.hpp"

#include <fcntl.h>
#include <boost/filesystem.hpp>

namespace fc::primitives::piece {
  PieceData::PieceData(const std::string &path_to_file) {
    fd_ = -1;

    if (boost::filesystem::exists(path_to_file)) {
      fd_ = open(path_to_file.c_str(), O_RDWR);
    }
  }

  int PieceData::getFd() const{
    return fd_;
  }

  PieceData::~PieceData() {
    if (fd_ != -1) {
      close(fd_);
    }
  }
}  // namespace fc::primitives::piece
