cmake_minimum_required(VERSION 3.12...3.31)
project(read_service CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(DownloadUserver)

if(DEFINED ENV{ENABLE_TESTSUITE} AND NOT $ENV{ENABLE_TESTSUITE})
    set(ENABLE_TESTSUITE OFF)
    set(USERVER_FEATURE_TESTSUITE OFF CACHE BOOL "" FORCE)
    set(USERVER_FEATURE_UTEST OFF CACHE BOOL "" FORCE)
    set(USERVER_FEATURE_POSTGRESQL OFF CACHE BOOL "" FORCE)
    message("ENABLE_TESTSUITE=OFF  ->  testsuite is disabled")
else()
    set(ENABLE_TESTSUITE ON)
endif()


find_package(
    userver
    COMPONENTS core
               postgresql
               grpc
    REQUIRED
)
if(NOT userver_FOUND)
    # Tries TRY_DIR first, falls back to downloading userver from GitHub using CPM.
    download_userver(TRY_DIR third_party/userver)
endif()

userver_setup_environment()

# Dependencies
set (SHARED_DIR ${CMAKE_SOURCE_DIR}/../../shared
    CACHE PATH "Path to shared directory")

add_subdirectory(${SHARED_DIR}/libs/jwt deps/jwt)

add_subdirectory(${SHARED_DIR}/libs/models deps/models)

find_package(AWSSDK CONFIG REQUIRED COMPONENTS s3)
add_subdirectory(${SHARED_DIR}/libs/aws deps/aws)

# Common sources
include_directories(src)

add_library(
    ${PROJECT_NAME}_objs OBJECT
    src/services/read_service.cpp
    src/components/metadata_repo.cpp
    src/components/blob_repo.cpp
    src/handlers/get_paste.cpp
    src/handlers/get_paste_details.cpp
    src/handlers/get_my_pastes.cpp
    src/clients/friends_client.cpp
)
target_link_libraries(
    ${PROJECT_NAME}_objs
    PUBLIC userver::core
           userver::postgresql
           userver::grpc
           shared_models
           jwt_verifier
           shared_aws
)

# grpc
userver_add_grpc_library(${PROJECT_NAME}-proto
    PROTOS friends.proto
    SOURCE_PATH ${SHARED_DIR}/proto    
)
target_link_libraries(${PROJECT_NAME}_objs PUBLIC ${PROJECT_NAME}-proto)

# The Service
add_executable(${PROJECT_NAME} src/main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE ${PROJECT_NAME}_objs)

# Unit Tests

# Benchmarks

# Functional testing
if(ENABLE_TESTSUITE)
    userver_testsuite_add_simple(
        REQUIREMENTS ${CMAKE_SOURCE_DIR}/tests/requirements.txt
    )
endif()

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