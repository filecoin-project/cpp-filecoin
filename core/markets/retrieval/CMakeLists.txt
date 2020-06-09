add_subdirectory(network)

add_library(retrieval_market_provider
    provider/retrieval_provider_impl.cpp
    provider/query_responder/query_responder_impl.cpp
    )
target_link_libraries(retrieval_market_provider
    retrieval_market_network
    address
    logger
    cbor
    cbor_stream
    )

add_library(retrieval_market_client
    client/retrieval_client_impl.cpp
    )
target_link_libraries(retrieval_market_client
    retrieval_market_network
    address
    logger
    cbor
    )
