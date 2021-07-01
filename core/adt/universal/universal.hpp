/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "common/outcome.hpp"
#include "primitives/cid/cid.hpp"

namespace fc::adt::universal {

  /**
   * Universal type is destined for general work with actor's types which are
   * different from version to version.
   *
   * To use this class, the T class must have the following hierarchy:
   * Base class: T
   * Inheritors: Tv0, Tv2, Tv3 ... TvN
   *
   * Where T is the common type with all fields of all versions (could be
   * abstract). And Tv0 - TvN are implementations of this type for each of
   * actor's version.
   */
  template <typename T>
  class Universal {
   public:
    virtual ~Universal() = default;

    /**
     * Loads an object from ipld.
     * Must be called before get() or set().
     */
    virtual outcome::result<void> load() = 0;

    /**
     * Returns the pointer to the common object.
     * @returns pointer to the object
     */
    virtual std::shared_ptr<T> get() const = 0;

    /**
     * Sets the object to ipld.
     */
    virtual outcome::result<CID> set() = 0;

    /**
     * Returns the actual CID of the object in ipld.
     * @returns CID
     */
    virtual const CID &getCid() const = 0;
  };

}  // namespace fc::adt::universal
