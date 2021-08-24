/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

namespace fc::common {

  /**
   * Returns slice of container between start and end.
   * Note: T must be a container that has begin() / end() iterators and
   * Constructor(InputIt first, InputIt last).
   * @param source - container
   * @param begin - start index of slice
   * @param end - last index of slice, when -1 - last index is source.end()
   * @return slice container that contains elements between begin and end from
   * source
   */
  template <typename T>
  inline T slice(const T &source, int begin, int end = -1) {
    typename T::const_iterator begin_it = source.begin() + begin;
    typename T::const_iterator end_it =
        end == -1 ? source.end() : source.begin() + end;
    return T(begin_it, end_it);
  }

}  // namespace fc::common
