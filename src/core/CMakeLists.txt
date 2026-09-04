add_library(bmapcore STATIC
  bmapparser.cpp
  imagesource.cpp
  verifier.cpp
  copier.cpp
)

target_include_directories(bmapcore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# libarchive target name differs across providers:
#   apt / Homebrew / upstream -> LibArchive::LibArchive
#   vcpkg                     -> unofficial-libarchive::libarchive (or libarchive::libarchive)
if(TARGET LibArchive::LibArchive)
  set(BMAP_ARCHIVE_TARGET LibArchive::LibArchive)
elseif(TARGET unofficial-libarchive::libarchive)
  set(BMAP_ARCHIVE_TARGET unofficial-libarchive::libarchive)
elseif(TARGET libarchive::libarchive)
  set(BMAP_ARCHIVE_TARGET libarchive::libarchive)
else()
  message(FATAL_ERROR "No usable libarchive imported target found")
endif()

target_link_libraries(bmapcore PUBLIC
  Qt${QT_VERSION_MAJOR}::Core
  ${BMAP_ARCHIVE_TARGET}
)
