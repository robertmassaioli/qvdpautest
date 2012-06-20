// kate: tab-indent on; indent-width 4; mixedindent off; indent-mode cstyle; remove-trailing-space on;
#ifndef MPEG4DECODER_H
#define MPEG4DECODER_H

#include <QList>

#include "vdpaucontext.h"

#ifdef VDP_DECODER_PROFILE_MPEG4_PART2_ASP



class MPEG4Frame
{
public:
	MPEG4Frame() {
		data = NULL;
	}
	~MPEG4Frame() {
		if ( data ) delete data;
	}

	VdpPictureInfoMPEG4Part2 info;
	int dataLength;
	uint8_t *data;
};



class MPEG4Decoder
{
public:
	MPEG4Decoder( VDPAUContext *v, QString dataDirectory );
	~MPEG4Decoder();
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
	QList< MPEG4Frame* > frames;
   QString dataFilename;
};
#endif // VDP_DECODER_PROFILE_MPEG4_PART2_ASP

#endif // MPEG4DECODER_H
