add_library(bmapplatform STATIC
  devicelister.cpp
  elevation.cpp
  rawdevice.cpp
  writersink.cpp
)

target_include_directories(bmapplatform PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(bmapplatform PUBLIC
  bmapcore
  Qt${QT_VERSION_MAJOR}::Core
)

if(APPLE)
  target_link_libraries(bmapplatform PRIVATE "-framework Security")
endif()
if(WIN32)
  target_link_libraries(bmapplatform PRIVATE shell32)
endif()
