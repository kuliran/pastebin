set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_C_COMPILER /usr/bin/clang)
set(VCPKG_CXX_COMPILER /usr/bin/clang++)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)

set(VCPKG_CMAKE_SYSTEM_NAME Linux)

# Add sanitizers for debug because userver debug is built with sanitizers
# otherwise we get undefined symbol errors
set(VCPKG_C_FLAGS_DEBUG "-fsanitize=address,undefined -fno-sanitize=function")
set(VCPKG_CXX_FLAGS_DEBUG "-fsanitize=address,undefined -fno-sanitize=function")
set(VCPKG_LINKER_FLAGS_DEBUG "-fsanitize=address,undefined")