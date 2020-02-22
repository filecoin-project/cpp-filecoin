add_library(filecoin_rle_plus_codec
    rle_plus_encoding_stream.cpp
    rle_plus_errors.cpp
    )

target_link_libraries(filecoin_rle_plus_codec
    Boost::boost
    filecoin_outcome
  )
