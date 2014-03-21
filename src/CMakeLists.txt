include(${QT_USE_FILE})
pkg_check_modules(VDPAU REQUIRED vdpau)
pkg_check_modules(LIBPCI REQUIRED libpci)

QT4_WRAP_CPP(WIDGET_MOC_OUTFILES vdpauwidget.h mainwidget.h)

include_directories(${VDPAU_INCLUDE_DIRS} ${LIBPCI_INCLUDE_DIRS})

add_executable(qvdpautest
   h264decoder.cpp
   main.cpp
   mainwidget.cpp
   mpeg4decoder.cpp
   mpegdecoder.cpp
   vc1decoder.cpp
   vdpaucontext.cpp
   vdpauwidget.cpp
   Utils.cpp
   PciDisplay.cpp
   ${WIDGET_MOC_OUTFILES}
   )

target_link_libraries(qvdpautest
   ${QT_LIBRARIES}
   ${VDPAU_LIBRARIES}
   ${LIBPCI_LIBRARIES}
   )

install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/qvdpautest DESTINATION bin)

