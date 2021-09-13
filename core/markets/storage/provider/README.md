# Storage Market Provider

Module is implemented
according [spec](https://spec.filecoin.io/#section-systems.filecoin_markets.storage_market.storage-provider).

Storage market provider is a part of Fuhon miner.

```
STORAGE_DEAL_UNKNOWN
 |
(ProviderEventOpen)
Handles incoming stream from storage market client.
 |
STORAGE_DEAL_VALIDATING
 |
(ProviderEventDealAccepted)
Verifies Deal proposal
 |
STORAGE_DEAL_WAITING_FOR_DATA
 |
(ProviderEventDataTransferCompleted)
Waits for data. Data transferred manually or through graphsync protocol.
 |
STORAGE_DEAL_VERIFY_DATA
 |
(ProviderEventVerifiedData)
Verifies data and generate piece commitment.
 |
STORAGE_DEAL_ENSURE_PROVIDER_FUNDS
 |
(ProviderEventFunded)
Ensures provider funds.
 |
STORAGE_DEAL_PUBLISH
 |
(ProviderEventDealPublishInitiated)
Publishes storage deal by a sorage market actor method PublishStorageDeals call.
 |
STORAGE_DEAL_PUBLISHING
 |
(ProviderEventDealPublished)
Adds piece to sector blocks.
 |
STORAGE_DEAL_STAGED
 |
(ProviderEventDealHandedOff)
 |
STORAGE_DEAL_SEALING
 |
(ProviderEventDealActivated)
Waits for miner actor commits a deal sector.
 |
STORAGE_DEAL_ACTIVE
 |
(ProviderEventDealCompleted)
 |
STORAGE_DEAL_EXPIRED
```
