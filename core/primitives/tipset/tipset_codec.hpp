/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_PRIMITIVES_TIPSET_TIPSET_CODEC_HPP
#define CPP_FILECOIN_CORE_PRIMITIVES_TIPSET_TIPSET_CODEC_HPP

#include "codec/cbor/cbor.hpp"
#include "common/outcome.hpp"
#include "common/outcome_throw.hpp"
#include "primitives/tipset/tipset.hpp"

namespace fc::primitives::tipset {

  /**
   * @brief cbor-encodes Tipset instance
   * @tparam Stream cbor-encoder stream type
   * @param s stream reference
   * @param tipset Tipset instance
   * @return stream reference
   */
  template <class Stream,
            typename = std::enable_if_t<
                std::remove_reference<Stream>::type::is_cbor_encoder_stream>>
  Stream &operator<<(Stream &&s, const Tipset &tipset) noexcept {
    return s << (s.list() << tipset.cids << tipset.blks << tipset.height);
  }

  /**
   * @brief cbor-decodes Tipset instance
   * @tparam Stream cbor-decoder stream type
   * @param s stream reference
   * @param tipset Tipset instance reference to decode into
   * @return stream reference
   */
  template <class Stream,
            typename = std::enable_if_t<
                std::remove_reference<Stream>::type::is_cbor_decoder_stream>>
  Stream &operator>>(Stream &&s, Tipset &tipset) {
    std::vector<CID> cids;
    std::vector<boost::optional<block::BlockHeader>> blks;

    s.list() >> cids >> blks >> tipset.height;
    tipset.cids = std::move(cids);
    tipset.blks = std::move(blks);
    return s;
  }

}  // namespace fc::primitives::tipset

#endif  // CPP_FILECOIN_CORE_PRIMITIVES_TIPSET_TIPSET_CODEC_HPP
