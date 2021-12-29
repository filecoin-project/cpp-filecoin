/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/piece/piece_data.hpp"

#include <boost/filesystem.hpp>

namespace fc::primitives::piece {
  PieceData::PieceData(const std::string &path_to_file, int flags)
      : isNullData(false) {
    fd_ = open(path_to_file.c_str(), flags, 0644);
  }

  PieceData::PieceData(PieceData &&other) noexcept
      : fd_(kUnopenedFileDescriptor), isNullData(other.isNullData) {
    fd_ = other.fd_;
    other.fd_ = kUnopenedFileDescriptor;
    other.isNullData = true;
  }

  PieceData::PieceData(int &pipe_fd) : fd_(pipe_fd), isNullData(false) {
    pipe_fd = kUnopenedFileDescriptor;
  }

  PieceData::PieceData() : fd_(kUnopenedFileDescriptor), isNullData(true) {}

  int PieceData::getFd() const {
    assert(not isNullData);
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

  bool PieceData::IsNullData() const {
    return isNullData;
  }

  PieceData &PieceData::operator=(PieceData &&other) noexcept {
    fd_ = other.fd_;
    other.fd_ = kUnopenedFileDescriptor;
    return *this;
  }
}  // namespace fc::primitives::piece
