HEADERS += \
	vdpauwidget.h\
	vdpaucontext.h \
	mpegdecoder.h \
	h264decoder.h \
	vc1decoder.h \
	mpeg4decoder.h \
	mainwidget.h
	
SOURCES += \
    vdpauwidget.cpp \
	vdpaucontext.cpp \
	mpegdecoder.cpp \
	h264decoder.cpp \
	vc1decoder.cpp \
    mpeg4decoder.cpp \
	main.cpp \
	mainwidget.cpp
	
LIBS += -lvdpau
