/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_MARKETS_RETRIEVAL_PROVIDER_QUERY_RESPONDER_IMPL_HPP
#define CPP_FILECOIN_MARKETS_RETRIEVAL_PROVIDER_QUERY_RESPONDER_IMPL_HPP

#include <memory>

#include <libp2p/connection/stream.hpp>
#include "api/api.hpp"
#include "common/libp2p/cbor_stream.hpp"
#include "common/logger.hpp"
#include "markets/retrieval/protocols/query_protocol.hpp"
#include "markets/retrieval/provider/retrieval_provider_types.hpp"
#include "storage/piece/piece_storage.hpp"

namespace fc::markets::retrieval::provider {
  class QueryResponderImpl
      : public std::enable_shared_from_this<QueryResponderImpl> {
   protected:
    using StreamShPtr = std::shared_ptr<libp2p::connection::Stream>;
    using PieceStorageShPtr =
        std::shared_ptr<::fc::storage::piece::PieceStorage>;
    using ApiShPtr = std::shared_ptr<api::Api>;
    using CborStreamShPtr = std::shared_ptr<common::libp2p::CborStream>;

   public:
    QueryResponderImpl(PieceStorageShPtr piece_storage,
                       ApiShPtr api,
                       common::Logger logger,
                       const ProviderConfig &config)
        : piece_storage_{std::move(piece_storage)},
          api_{std::move(api)},
          logger_{std::move(logger)},
          provider_config_{config} {}

    void onNewRequest(const CborStreamShPtr &stream);

   private:
    PieceStorageShPtr piece_storage_;
    ApiShPtr api_;
    common::Logger logger_;
    const ProviderConfig &provider_config_;

    QueryItemStatus getItemStatus(const CID &payload_cid,
                                  const CID &piece_cid) const;

    void closeNetworkStream(StreamShPtr stream) const;
  };
}  // namespace fc::markets::retrieval::provider

#endif
