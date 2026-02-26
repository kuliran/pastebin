# In the calling CMakeLists.txt add:
#
# find_package(AWSSDK CONFIG REQUIRED COMPONENTS s3)
# add_subdirectory(${SHARED_DIR}/libs/aws deps/aws)
#

add_library(shared_aws
    ${CMAKE_CURRENT_SOURCE_DIR}/aws_sdk_component.cpp
)
target_include_directories(shared_aws PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${AWSSDK_INCLUDE_DIRS}
)
target_link_libraries(shared_aws PUBLIC
    userver::core
    ${AWSSDK_LIBRARIES}
)
