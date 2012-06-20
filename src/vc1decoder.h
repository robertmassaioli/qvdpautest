// kate: tab-indent on; indent-width 4; mixedindent off; indent-mode cstyle; remove-trailing-space on;
#ifndef VC1DECODER_H
#define VC1DECODER_H

#include <QList>

#include "vdpaucontext.h"



class VC1Frame
{
public:
	VC1Frame() {
		data = NULL;
	}
	~VC1Frame() {
		if ( data ) delete data;
	}
	VdpPictureInfoVC1 info;
	int dataLength;
	uint8_t *data;
};



class VC1Decoder
{
public:
	VC1Decoder( VDPAUContext *v, QString dataDirectory );
	~VC1Decoder();
	bool init();
	VdpVideoSurface getNextFrame();
	
	uint32_t width, height;
	double ratio;
	
private:
	VDPAUContext *vc;
	VdpDecoderProfile profile;
	VdpDecoder decoder;
	VdpVideoSurface surfaces[3];
	VdpVideoSurface backwardRef, forwardRef, currentSurface;
	int currentFrame;
	QList< VC1Frame* > frames;	
   QString dataFilename;
};
#endif // VC1DECODER_H
