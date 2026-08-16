set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE static)
set(VCPKG_LIBRARY_LINKAGE static)

# UTBot's portable package is a Release build. The built-in static Windows
# triplet builds both Debug and Release copies of every port, which nearly
# doubles the cold gRPC build without adding anything to the distribution.
set(VCPKG_BUILD_TYPE release)
