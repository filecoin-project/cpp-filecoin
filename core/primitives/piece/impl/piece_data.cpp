/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "primitives/piece/piece_data.hpp"

#include <boost/filesystem.hpp>

namespace fc::primitives::piece {
  PieceData::PieceData(const std::string &path_to_file, int flags) {
    fd_ = open(path_to_file.c_str(), flags, 0644);
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

  PieceData &PieceData::operator=(PieceData &&other) noexcept {
    fd_ = other.fd_;
    other.fd_ = kUnopenedFileDescriptor;
    return *this;
  }

  int PieceData::release(){
    int temp = fd_;
    fd_ = kUnopenedFileDescriptor;
    return temp;
  }

  const std::array<std::string, 2> ReaderType::types{"push", "null"};

   const std::string ReaderType::toString() const {
    return types.at(reader_type);
  }

}  // namespace fc::primitives::piece
