/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/piece/piece_data.hpp"

#include <fcntl.h>
#include <boost/filesystem.hpp>

namespace fc::primitives::piece {
  PieceData::PieceData(const std::string &path_to_file)
      : fd_(kUnopenedFileDescriptor) {
    if (boost::filesystem::exists(path_to_file)) {
      fd_ = open(path_to_file.c_str(), O_RDWR);
    }
  }

  int PieceData::getFd() const {
    return fd_;
  }

  PieceData::~PieceData() {
    if (fd_ != kUnopenedFileDescriptor) {
      close(fd_);
    }
  }

  bool PieceData::isOpened() const {
    return fd_ != kUnopenedFileDescriptor;
  }

  PieceData::PieceData(PieceData &&other) noexcept
      : fd_(kUnopenedFileDescriptor) {
    fd_ = other.fd_;
    other.fd_ = kUnopenedFileDescriptor;
  }

  PieceData::PieceData(int &pipe_fd) : fd_(pipe_fd) {
    pipe_fd = kUnopenedFileDescriptor;
  }
}  // namespace fc::primitives::piece
