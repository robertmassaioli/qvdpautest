// kate: tab-indent on; indent-width 4; mixedindent off; indent-mode cstyle; remove-trailing-space on;
#ifndef MPEGDECODER_H
#define MPEGDECODER_H

#include <QList>

#include "vdpaucontext.h"



class MPEGFrame
{
   public:
      MPEGFrame() {
         data = NULL;
      }
      ~MPEGFrame() {
         if ( data ) delete data;
      }
      VdpPictureInfoMPEG1Or2 info;
      int dataLength;
      uint8_t *data;
};



class MPEGDecoder
{
   public:
      MPEGDecoder( VDPAUContext *v, QString filename="mpghd.dat" );
      ~MPEGDecoder();
      bool init( bool decodeOnly=false );
      VdpVideoSurface getNextFrame();
      QList< VdpVideoSurface > getOrderedFrames();

      uint32_t width, height;
      double ratio;

   private:
      VDPAUContext *vc;
      VdpDecoderProfile profile;
      VdpDecoder decoder;
      VdpVideoSurface surfaces[NUMSURFACES];
      VdpVideoSurface backwardRef, forwardRef, currentSurface;
      int currentFrame;
      QList< MPEGFrame* > frames;
      QString testFile;
      bool onlyDecode;
};
#endif // MPEGDECODER_H
