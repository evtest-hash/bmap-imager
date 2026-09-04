add_executable(bmapimager
  main.cpp
  mainwindow.cpp
  flashworker.cpp
)

target_link_libraries(bmapimager PRIVATE
  bmapcore
  bmapplatform
  Qt${QT_VERSION_MAJOR}::Widgets
)
