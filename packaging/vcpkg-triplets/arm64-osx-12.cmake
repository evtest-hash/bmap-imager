# libarchive (and its compressors) for the macOS .app.
#
# Two things this pins that the stock arm64-osx triplet does not:
#
#   VCPKG_OSX_DEPLOYMENT_TARGET  Homebrew bottles are always built for the
#       runner's own OS, so a brew libarchive from a macos-26 runner carries
#       minos 26 and the bundle refuses to load anywhere older. Pin it to the
#       same floor as the app itself.
#
# Linkage stays dynamic: that is the path macdeployqt already handles
# correctly for the main executable, and the helper is fixed by passing it to
# macdeployqt explicitly rather than by changing how libarchive is linked.
set(VCPKG_TARGET_ARCHITECTURE arm64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE dynamic)
set(VCPKG_CMAKE_SYSTEM_NAME Darwin)
set(VCPKG_OSX_DEPLOYMENT_TARGET 12.0)
