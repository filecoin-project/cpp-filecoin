/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

// Copyright © 2017-2019 Dmitriy Khaustov
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Author: Dmitriy Khaustov aka xDimon
// Contacts: khaustov.dm@gmail.com
// File created on: 2017.03.26

// HttpUri.hpp

#pragma once

#include <string>

namespace fc::common {
  class HttpUri final {
   public:
    enum class Scheme { UNDEFINED, HTTP, HTTPS };

    HttpUri() = default;

    explicit HttpUri(const std::string &uri);

    virtual ~HttpUri() = default;

    void parse(const std::string &string);

    const std::string &str() const;

    Scheme scheme() const {
      return scheme_;
    }

    void setScheme(Scheme scheme) {
      scheme_ = scheme;
      if (port_ == 0) {
        if (scheme_ == Scheme::HTTP) {
          port_ = 80;
        } else if (scheme_ == Scheme::HTTPS) {
          port_ = 443;
        }
      }
    }

    const std::string &host() const {
      return host_;
    }

    void setHost(const std::string &host) {
      host_ = host;
    }

    uint16_t port() const {
      return port_;
    }

    void setPort(uint16_t port) {
      port_ = port;
    }

    const std::string &path() const {
      return path_;
    }

    void setPath(const std::string &path) {
      path_ = path;
    }

    bool hasQuery() const {
      return hasQuery_;
    }

    const std::string &query() const {
      return query_;
    }

    void setQuery(const std::string &query) {
      query_ = query;
      if (!query_.empty()) {
        hasQuery_ = true;
      }
    }

    bool hasFragment() const {
      return hasFragment_;
    }

    const std::string &fragment() const {
      return fragment_;
    }

    void setFragment(const std::string &fragment) {
      fragment_ = fragment;
      if (!fragment_.empty()) {
        hasFragment_ = true;
      }
    }

    static std::string urldecode(const std::string &input);

    static std::string urlencode(const std::string &input);

   private:
    void parsePath(const char *s, const char *end);

    Scheme scheme_ = Scheme::UNDEFINED;
    std::string host_;
    uint16_t port_ = 0;
    std::string path_;
    bool hasQuery_ = false;
    std::string query_;
    bool hasFragment_ = false;
    std::string fragment_;
    mutable std::string thisAsString_;
  };
}  // namespace fc::common
