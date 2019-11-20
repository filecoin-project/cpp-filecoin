/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_PRIMITIVES_MULTIPRECISION_HPP
#define CPP_FILECOIN_CORE_PRIMITIVES_MULTIPRECISION_HPP

#include <type_traits>

#include <boost/multiprecision/cpp_int.hpp>

/**
 * Extend std::type_traits for support boost_multiprecision::int
 */
template <>
struct std::is_signed<boost::multiprecision::int128_t> : public true_type {};
template <>
struct std::is_signed<boost::multiprecision::int256_t> : public true_type {};
template <>
struct std::is_signed<boost::multiprecision::int512_t> : public true_type {};
template <>
struct std::is_signed<boost::multiprecision::int1024_t> : public true_type {};

/**
 * Extend std::type_traits for support boost_multiprecision::uint
 */
template <>
struct std::is_unsigned<boost::multiprecision::uint128_t> : public true_type {};
template <>
struct std::is_unsigned<boost::multiprecision::uint256_t> : public true_type {};
template <>
struct std::is_unsigned<boost::multiprecision::uint512_t> : public true_type {};
template <>
struct std::is_unsigned<boost::multiprecision::uint1024_t> : public true_type {
};

#endif  // CPP_FILECOIN_CORE_PRIMITIVES_MULTIPRECISION_HPP
