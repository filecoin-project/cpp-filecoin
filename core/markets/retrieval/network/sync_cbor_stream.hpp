/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_MARKETS_RETRIEVAL_NETWORK_SYNC_CBOR_STREAM_HPP
#define CPP_FILECOIN_CORE_MARKETS_RETRIEVAL_NETWORK_SYNC_CBOR_STREAM_HPP

#include <future>

#include "common/libp2p/cbor_stream.hpp"
#include "markets/retrieval/network/async_operation.hpp"

namespace fc::markets::retrieval::network {
  enum class SyncCborStreamErrors { READ_ERROR = 1, WRITE_ERROR };
}
OUTCOME_HPP_DECLARE_ERROR(fc::markets::retrieval::network, SyncCborStreamErrors)

namespace fc::markets::retrieval::network {
  class SyncCborStream : public std::enable_shared_from_this<SyncCborStream> {
   protected:
    using CborStream = common::libp2p::CborStream;
    using StreamShPtr = std::shared_ptr<libp2p::connection::Stream>;

   public:
    SyncCborStream(StreamShPtr stream)
        : cbor_stream_{std::make_shared<CborStream>(std::move(stream))} {}

    template <class T>
    outcome::result<std::shared_ptr<T>> read() const {
      std::shared_ptr<T> data;
      OUTCOME_TRY(AsyncOperation::run([&data, self = shared_from_this()](
                                          AsyncOperation::Operation operation) {
        self->cbor_stream_->read<T>([&data,
                                     operation](outcome::result<T> result) {
          if (result.has_value()) {
            data = std::make_shared<T>(std::move(result.value()));
            operation->set_value();
          } else {
            operation->set_exception(std::make_exception_ptr(std::system_error{
                make_error_code(SyncCborStreamErrors::READ_ERROR)}));
          }
        });
      }));
      return data;
    }

    template <class T>
    outcome::result<void> write(T data) const {
      return AsyncOperation::run([&data, self = shared_from_this()](
                                     AsyncOperation::Operation operation) {
        self->cbor_stream_->write(
            data, [operation](outcome::result<size_t> result) {
              if (result.has_value()) {
                operation->set_value();
              } else {
                operation->set_exception(
                    std::make_exception_ptr(std::system_error{
                        make_error_code(SyncCborStreamErrors::WRITE_ERROR)}));
              }
            });
      });
    }

   private:
    std::shared_ptr<CborStream> cbor_stream_;
  };

}  // namespace fc::markets::retrieval::network

#endif
