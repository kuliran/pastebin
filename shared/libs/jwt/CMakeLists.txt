add_library(jwt_verifier INTERFACE)
target_include_directories(jwt_verifier INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/verifier>
)
target_link_libraries(jwt_verifier INTERFACE userver::core)

add_library(jwt_issuer INTERFACE)
target_include_directories(jwt_issuer INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/issuer>
)