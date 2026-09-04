add_library(bmapcore STATIC
  bmapparser.cpp
  imagesource.cpp
  verifier.cpp
  copier.cpp
)

target_include_directories(bmapcore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(bmapcore PUBLIC
  Qt${QT_VERSION_MAJOR}::Core
  LibArchive::LibArchive
)
