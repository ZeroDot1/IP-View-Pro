# ═══════════════════════════════════════════════════════════════════════════════
#  IPView Pro v2.15.0 — tests/CMakeLists.txt
#
#  Builds the IPViewTests binary from the sources in this directory.
#  The tests are header-only test cases + a tiny main.cpp driver.
# ═══════════════════════════════════════════════════════════════════════════════

add_executable(IPViewTests
    main.cpp
    test_safetext.cpp
    test_concepts.cpp
    test_contracts.cpp
    test_timeouts.cpp
    test_error.cpp
    test_security.cpp
    test_safeprocess.cpp
)

target_link_libraries(IPViewTests
    PRIVATE
        Qt6::Core
        Qt6::Network
)

target_include_directories(IPViewTests
    PRIVATE
        ${CMAKE_SOURCE_DIR}
)

set_target_properties(IPViewTests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests"
)

enable_testing()
add_test(NAME IPViewTests COMMAND IPViewTests)
