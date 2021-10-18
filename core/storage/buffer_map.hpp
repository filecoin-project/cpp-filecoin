/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

/**
 * This file contains:
 *  - BufferMap - contains key-value bindings of Buffers
 *  - PersistentBufferMap - stores key-value bindings on filesystem or remote
 * connection.
 */

#include <gsl/span>

#include "common/bytes.hpp"
#include "storage/face/generic_map.hpp"
#include "storage/face/persistent_map.hpp"
#include "storage/face/write_batch.hpp"

namespace fc::storage {

  using BufferMap = face::GenericMap<Bytes, Bytes>;

  using BufferBatch = face::WriteBatch<Bytes, Bytes>;

  using PersistentBufferMap = face::PersistentMap<Bytes, Bytes>;

  using BufferMapCursor = face::MapCursor<Bytes, Bytes>;

}  // namespace fc::storage
