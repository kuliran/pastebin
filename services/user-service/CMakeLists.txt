cmake_minimum_required(VERSION 3.12...3.31)
project(user_service CXX)

set (SHARED_DIR ${CMAKE_SOURCE_DIR}/../../shared
    CACHE PATH "Path to shared directory")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(DownloadUserver)

find_package(
    userver
    COMPONENTS core
               mongo
               postgresql
    QUIET
)
if(NOT userver_FOUND)
    # Tries TRY_DIR first, falls back to downloading userver from GitHub using CPM.
    download_userver(TRY_DIR third_party/userver)
endif()

userver_setup_environment()

# Dependencies
add_subdirectory(${SHARED_DIR}/libs/jwt deps/jwt)

find_library(ARGON2_LIB argon2 REQUIRED)

# Common sources
include_directories(src)

add_library(
    ${PROJECT_NAME}_objs OBJECT
    src/services/auth_service.cpp
    src/components/auth_repo.cpp
    src/components/cookie_factory.cpp
    src/utils/crypto.cpp
    src/handlers/signup.cpp
    src/handlers/login.cpp
    src/handlers/refresh.cpp
)
target_link_libraries(
    ${PROJECT_NAME}_objs
    PUBLIC userver::core
           userver::mongo
           userver::postgresql
           jwt_verifier
           jwt_issuer
           ${ARGON2_LIB}
)


# The Service
add_executable(${PROJECT_NAME} src/main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE ${PROJECT_NAME}_objs)

# Unit Tests

# Benchmarks

# Functional testing
userver_testsuite_add_simple(
    REQUIREMENTS ${CMAKE_SOURCE_DIR}/tests/requirements.txt
)

# Install
include(GNUInstallDirs)

if(DEFINED ENV{PREFIX})
    message(STATUS "Set install prefix: $ENV{PREFIX}")
    file(TO_CMAKE_PATH "$ENV{PREFIX}" PREFIX_PATH)
    set(CMAKE_INSTALL_PREFIX "${PREFIX_PATH}")
endif()

file(GLOB CONFIGS_FILES ${CMAKE_CURRENT_SOURCE_DIR}/configs/*.yaml ${CMAKE_CURRENT_SOURCE_DIR}/configs/*.json)

install(
    TARGETS ${PROJECT_NAME}
    DESTINATION ${CMAKE_INSTALL_BINDIR}
    COMPONENT ${PROJECT_NAME}
)
install(
    FILES ${CONFIGS_FILES}
    DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/${PROJECT_NAME}
    COMPONENT ${PROJECT_NAME}
)