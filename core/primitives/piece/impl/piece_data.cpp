/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/piece/piece_data.hpp"

#include <boost/filesystem.hpp>

namespace fc::primitives::piece {
  PieceData::PieceData(const std::string &path_to_file, int flags)
      : is_null_data_(false) {
    fd_ = open(path_to_file.c_str(), flags, 0644);
  }

  PieceData::PieceData(PieceData &&other) noexcept
      : fd_(kUnopenedFileDescriptor), is_null_data_(other.is_null_data_) {
    fd_ = other.fd_;
    other.fd_ = kUnopenedFileDescriptor;
    other.is_null_data_ = true;
  }

  PieceData::PieceData(int &pipe_fd) : fd_(pipe_fd), is_null_data_(false) {
    pipe_fd = kUnopenedFileDescriptor;
  }

  PieceData::PieceData() : fd_(kUnopenedFileDescriptor), is_null_data_(true) {}

  int PieceData::getFd() const {
    assert(not is_null_data_);
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

  bool PieceData::isNullData() const {
    return is_null_data_;
  }

  PieceData &PieceData::operator=(PieceData &&other) noexcept {
    fd_ = other.fd_;
    other.fd_ = kUnopenedFileDescriptor;
    return *this;
  }
}  // namespace fc::primitives::piece
