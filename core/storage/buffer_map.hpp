/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_BUFFER_MAP_HPP
#define CPP_FILECOIN_BUFFER_MAP_HPP

/**
 * This file contains:
 *  - BufferMap - contains key-value bindings of Buffers
 *  - PersistentBufferMap - stores key-value bindings on filesystem or remote
 * connection.
 */

#include <gsl/span>

#include "common/buffer.hpp"
#include "storage/face/generic_map.hpp"
#include "storage/face/persistent_map.hpp"
#include "storage/face/write_batch.hpp"

namespace fc::storage {

  using Buffer = common::Buffer;

  using BufferMap = face::GenericMap<Buffer, Buffer>;

  using BufferBatch = face::WriteBatch<Buffer, Buffer>;

  using PersistentBufferMap = face::PersistentMap<Buffer, Buffer>;

  using BufferMapCursor = face::MapCursor<Buffer, Buffer>;

}  // namespace fc::storage

#endif  // CPP_FILECOIN_BUFFER_MAP_HPP
