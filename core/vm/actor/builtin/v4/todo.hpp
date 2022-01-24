/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <boost/preprocessor/stringize.hpp>
#include <stdexcept>

#define TODO_ACTORS_V4()                            \
  throw std::logic_error("TODO_ACTORS_V4 " __FILE__ \
                         ":" BOOST_PP_STRINGIZE(__LINE__))

#define TODO_ACTORS_V5()                            \
  throw std::logic_error("TODO_ACTORS_V5 " __FILE__ \
                         ":" BOOST_PP_STRINGIZE(__LINE__))

#define TODO_ACTORS_V6()                            \
  throw std::logic_error("TODO_ACTORS_V6 " __FILE__ \
                         ":" BOOST_PP_STRINGIZE(__LINE__))

#define TODO_ACTORS_V7()                            \
  throw std::logic_error("TODO_ACTORS_V7 " __FILE__ \
                         ":" BOOST_PP_STRINGIZE(__LINE__))