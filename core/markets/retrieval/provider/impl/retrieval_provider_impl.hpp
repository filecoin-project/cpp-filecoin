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
  using primitives::SectorNumber;
  using primitives::piece::PieceData;
  using primitives::piece::UnpaddedPieceSize;
  using GsResStatus = ::fc::storage::ipfs::graphsync::ResponseStatusCode;

  struct DealState {
    DealState(PeerDtId pdtid, PeerGsId pgsid, const DealProposal &proposal)
        : proposal{proposal},
          state{proposal.params},
          pdtid{std::move(pdtid)},
          pgsid{std::move(pgsid)} {}

    DealProposal proposal;
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

    void onProposal(const PeerDtId &pdtid,
                    const PeerGsId &pgsid,
                    const DealProposal &proposal);
    void onPayment(const std::shared_ptr<DealState> &deal,
                   const DealPayment &payment);
    void doUnseal(const std::shared_ptr<DealState> &deal);
    void doBlocks(const std::shared_ptr<DealState> &deal);
    void doComplete(const std::shared_ptr<DealState> &deal);
    bool hasOwed(const std::shared_ptr<DealState> &deal);
    void doFail(const std::shared_ptr<DealState> &deal, std::string error);

    void start() override;

    void setPricePerByte(TokenAmount amount) override;

    void setPaymentInterval(uint64_t payment_interval,
                            uint64_t payment_interval_increase) override;

   private:
    /**
     * Handle query stream
     * @param stream - cbor stream
     */
    void handleQuery(const std::shared_ptr<CborStream> &stream);

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
    void respondErrorQueryResponse(const std::shared_ptr<CborStream> &stream,
                                   const std::string &message);

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
