
add_executable(compacter-test
    test.cpp
    )
target_link_libraries(compacter-test
    compacter
    in_memory_storage
    )
set_target_properties(compacter-test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    )
