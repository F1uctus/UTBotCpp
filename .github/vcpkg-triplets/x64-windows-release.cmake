set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE dynamic)

# Host tools such as protoc and grpc_cpp_plugin only run while UTBot is being
# built. A Release-only host triplet avoids compiling their unused Debug copy.
set(VCPKG_BUILD_TYPE release)
