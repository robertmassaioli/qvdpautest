// kate: tab-indent on; indent-width 4; mixedindent off; indent-mode cstyle; remove-trailing-space on;
#ifndef H264DECODER_H
#define H264DECODER_H

#include <QList>

#include "vdpaucontext.h"



class H264Frame
{
public:
	H264Frame() {
		data = NULL;
	}
	~H264Frame() {
		if ( data ) delete data;
	}
	VdpPictureInfoH264 info;
	int dataLength;
	uint8_t *data;
};



class H264Decoder
{
public:
	H264Decoder( VDPAUContext *v, QString filename="h264hd.dat" );
	~H264Decoder();
	bool init();
	VdpVideoSurface getNextFrame();
	
	uint32_t width, height;
	double ratio;
	
private:
	VDPAUContext *vc;
	VdpDecoderProfile profile;
	VdpDecoder decoder;
	VdpVideoSurface surfaces[NUMSURFACES];
	int currentSurface;
	int currentFrame;
	QList< H264Frame* > frames;
	int refframes[FRAMESINSAMPLE];
	QString testFile;
};
#endif // H264DECODER_H
