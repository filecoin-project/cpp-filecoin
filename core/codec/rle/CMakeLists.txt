add_library(rle_plus_codec
    rle_plus_encoding_stream.cpp
    rle_plus_errors.cpp
    )

target_link_libraries(rle_plus_codec
    Boost::boost
    outcome
  )
