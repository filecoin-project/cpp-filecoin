/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "index_lock.hpp"

namespace fc::sector_storage::stores {
  IndexLock::Lock::~Lock() {
    if (index) {
      index->unlock(*this);
    }
  }

  bool IndexLock::Sector::canLock(SectorFileType read,
                                  SectorFileType write) const {
    if ((read | write) & this->write) {
      return false;
    }
    for (auto i{0u}; i < kSectorFileTypeBits; ++i) {
      if (write & (1 << i) && this->read[i]) {
        return false;
      }
    }
    return true;
  }

  bool IndexLock::lock(IndexLock::Lock &lock, bool wait) {
    assert(!lock.index);
    if (lock.read || lock.write) {
      std::unique_lock index_lock{mutex};
      auto &sector{sectors[lock.sector]};
      ++sector.refs;
      index_lock.unlock();
      std::unique_lock sector_lock{sector.mutex};
      while (true) {
        if (sector.canLock(lock.read, lock.write)) {
          for (auto i{0u}; i < kSectorFileTypeBits; ++i) {
            if (lock.read & (1 << i)) {
              ++sector.read[i];
            }
          }
          sector.write = static_cast<SectorFileType>(sector.write | lock.write);
          lock.index = shared_from_this();
          return true;
        } else if (wait) {
          sector.cv.wait(sector_lock);
        } else {
          break;
        }
      }
    }
    return false;
  }

  void IndexLock::unlock(Lock &lock) {
    assert(lock.index.get() == this);
    lock.index.reset();
    std::unique_lock index_lock{mutex};
    auto &sector{sectors.at(lock.sector)};
    std::unique_lock sector_lock{sector.mutex};
    for (auto i{0u}; i < kSectorFileTypeBits; ++i) {
      if (lock.read & (1 << i)) {
        --sector.read[i];
      }
    }
    sector.write = static_cast<SectorFileType>(sector.write & ~lock.write);
    --sector.refs;
    if (!sector.refs) {
      sectors.erase(lock.sector);
    }
    sector.cv.notify_all();
  }
}  // namespace fc::sector_storage::stores
