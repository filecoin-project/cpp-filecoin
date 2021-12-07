/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "api/full_node/node_api.hpp"
#include "common/io_thread.hpp"
#include "common/libp2p/cbor_stream.hpp"
#include "common/logger.hpp"
#include "data_transfer/dt.hpp"
#include "markets/common.hpp"
#include "markets/retrieval/protocols/query_protocol.hpp"
#include "markets/retrieval/protocols/retrieval_protocol.hpp"
#include "markets/retrieval/provider/retrieval_provider.hpp"
#include "miner/miner.hpp"
#include "storage/ipld/memory_indexed_car.hpp"
#include "storage/ipld/traverser.hpp"
#include "storage/map_prefix/prefix.hpp"
#include "storage/piece/piece_storage.hpp"

namespace fc::markets::retrieval::provider {
  using common::libp2p::CborStream;
  using data_transfer::DataTransfer;
  using data_transfer::DataTransferMessage;
  using data_transfer::DataTransferResponse;
  using data_transfer::PeerDtId;
  using data_transfer::PeerGsId;
  using ::fc::miner::Miner;
  using ::fc::sector_storage::Manager;
  using ::fc::storage::OneKey;
  using ::fc::storage::ipld::traverser::Traverser;
  using ::fc::storage::piece::PieceInfo;
  using ::fc::storage::piece::PieceStorage;
  using libp2p::Host;
  using primitives::BigInt;
  using primitives::DealId;
  using primitives::SectorNumber;
  using primitives::piece::PieceData;
  using primitives::piece::UnpaddedPieceSize;
  using GsResStatus = ::fc::storage::ipfs::graphsync::ResponseStatusCode;

  struct DealState {
    DealState(PeerDtId pdtid,
              PeerGsId pgsid,
              std::shared_ptr<DealProposal> proposal)
        : proposal{std::move(proposal)},
          state{this->proposal->params},
          pdtid{std::move(pdtid)},
          pgsid{std::move(pgsid)} {}

    std::shared_ptr<DealProposal> proposal;
    State state;
    PeerDtId pdtid;
    PeerGsId pgsid;
    bool unsealed{false};
    std::shared_ptr<MemoryIndexedCar> ipld;
    std::optional<Traverser> traverser;
  };

  class RetrievalProviderImpl
      : public RetrievalProvider,
        public std::enable_shared_from_this<RetrievalProviderImpl> {
   public:
    RetrievalProviderImpl(std::shared_ptr<Host> host,
                          std::shared_ptr<DataTransfer> datatransfer,
                          std::shared_ptr<api::FullNodeApi> api,
                          std::shared_ptr<PieceStorage> piece_storage,
                          std::shared_ptr<OneKey> config_key,
                          std::shared_ptr<Manager> sealer,
                          std::shared_ptr<Miner> miner);

    RetrievalAsk getAsk() const override;
    void setAsk(const RetrievalAsk &ask) override;
    void start() override;
    void setPricePerByte(TokenAmount amount) override;
    void setPaymentInterval(uint64_t payment_interval,
                            uint64_t payment_interval_increase) override;

   private:
    /**
     * Handle query stream
     * @param stream - cbor stream
     */
    template <typename QueryType, typename ResponseType>
    void handleQuery(const std::shared_ptr<CborStream> &stream) {
      stream->read<QueryType>(
          [self{shared_from_this()}, stream](auto request_res) {
            if (request_res.has_error()) {
              self->respondErrorQueryResponse<ResponseType>(
                  stream, request_res.error().message());
              return;
            }
            auto response_res = self->makeQueryResponse(request_res.value());
            if (response_res.has_error()) {
              self->respondErrorQueryResponse<ResponseType>(
                  stream, response_res.error().message());
              return;
            }
            stream->write(ResponseType{response_res.value()},
                          [self, stream](auto written) {
                            if (written.has_error()) {
                              self->logger_->error("Error while error response "
                                                   + written.error().message());
                            }
                            closeStreamGracefully(stream, self->logger_);
                          });
          });
    }

    /**
     * Look for piece by cid and make response
     * @param query with piece look for
     * @return network response
     */
    outcome::result<QueryResponse> makeQueryResponse(const QueryRequest &query);

    /**
     * Send error response for query and close stream
     * @param stream to send and close
     * @param message error to send
     */
    template <typename ResponseType>
    void respondErrorQueryResponse(const std::shared_ptr<CborStream> &stream,
                                   const std::string &message) {
      ResponseType response;
      response.response_status = QueryResponseStatus::kQueryResponseError;
      response.item_status = QueryItemStatus::kQueryItemUnknown;
      response.message = message;
      stream->write(response, [self{shared_from_this()}, stream](auto written) {
        if (written.has_error()) {
          self->logger_->error("Error while error response "
                               + written.error().message());
        }
        closeStreamGracefully(stream, self->logger_);
      });
    }

    void onProposal(const PeerDtId &pdtid,
                    const PeerGsId &pgsid,
                    const std::shared_ptr<DealProposal> &proposal);
    void onPayment(const std::shared_ptr<DealState> &deal,
                   const DealPayment &payment);

    template <typename DealResponseType>
    void acceptDataTransferPoolWithResponse(const PeerDtId &pdtid,
                                            const PeerGsId &pgsid,
                                            DealResponse deal_response) {
      datatransfer_->acceptPull(
          pdtid,
          pgsid,
          DealResponseType::type,
          codec::cbor::encode(DealResponseType{deal_response}).value());
    }

    void acceptDatatransferPull(const std::string &protocol,
                                const PeerDtId &pdtid,
                                const PeerGsId &pgsid,
                                const DealResponse &deal_response);

    template <typename DealResponseType>
    void respondDataTransferWithResponse(const PeerDtId &pdtid,
                                         const PeerGsId &pgsid,
                                         bool full_content,
                                         const DealResponse &deal_response) {
      datatransfer_->gs->postResponse(
          pgsid,
          {full_content ? GsResStatus::RS_FULL_CONTENT
                        : GsResStatus::RS_PARTIAL_RESPONSE,
           {DataTransfer::makeExt(DataTransferMessage{DataTransferResponse{
               full_content ? data_transfer::MessageType::kCompleteMessage
                            : data_transfer::MessageType::kVoucherResultMessage,
               true,
               false,
               pdtid.id,
               CborRaw{codec::cbor::encode<DealResponseType>({deal_response})
                           .value()},
               DealResponseType::type,
           }})},
           {}});
    }

    void respondDataTransfer(const std::string &protocol,
                             const PeerDtId &pdtid,
                             const PeerGsId &pgsid,
                             bool full_content,
                             const DealResponse &deal_response);

    template <typename DealResponseType>
    void rejectDataTransferWithResponse(const PeerDtId &pdtid,
                                        const PeerGsId &pgsid,
                                        DealId deal_id,
                                        DealStatus status,
                                        const std::string &message) {
      datatransfer_->rejectPull(
          pdtid,
          pgsid,
          DealResponseType::type,
          CborRaw{codec::cbor::encode(
                      DealResponseType{{status, deal_id, {}, message}})
                      .value()});
    }

    void rejectDatatransferPull(const std::string &protocol,
                                const PeerDtId &pdtid,
                                const PeerGsId &pgsid,
                                primitives::DealId deal_id,
                                DealStatus status,
                                const std::string &message);
    void doUnseal(const std::shared_ptr<DealState> &deal);
    void doBlocks(const std::shared_ptr<DealState> &deal);
    void doComplete(const std::shared_ptr<DealState> &deal);
    bool hasOwed(const std::shared_ptr<DealState> &deal);
    void doFail(const std::shared_ptr<DealState> &deal,
                const std::string &error);

    /**
     * Unseal sector
     * @param sector_id - id of sector
     * @param offset - offset of piece
     * @param size - size of piece
     * @return Read PieceData
     */
    outcome::result<void> unsealSector(SectorNumber sector_id,
                                       UnpaddedPieceSize offset,
                                       UnpaddedPieceSize size,
                                       const std::string &output_path);

    std::shared_ptr<Host> host_;
    std::shared_ptr<DataTransfer> datatransfer_;
    std::shared_ptr<api::FullNodeApi> api_;
    std::shared_ptr<PieceStorage> piece_storage_;
    std::shared_ptr<OneKey> config_key_;
    RetrievalAsk config_;
    std::shared_ptr<Manager> sealer_;
    std::shared_ptr<Miner> miner_;
    common::Logger logger_ = common::createLogger("RetrievalProvider");
    IoThread io_;
  };
}  // namespace fc::markets::retrieval::provider
