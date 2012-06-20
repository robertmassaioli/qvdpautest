// kate: tab-indent on; indent-width 4; mixedindent off; indent-mode cstyle; remove-trailing-space on;
#ifndef VDPAUWIDGET_H
#define VDPAUWIDGET_H

#include <QWidget>
#include <QThread>

#include "vdpaucontext.h"



class VdpauThread : public QThread
{
   public:
      VdpauThread( VDPAUContext *v, QList< VdpVideoSurface > list, VdpVideoMixer m, VdpOutputSurface ms, int w, int h ) {
         running = false;
         slist = list;
         mixer = m;
         mixerSurface = ms;
         width = w;
         height = h;
         vc = v;
      }
      void run();

      bool running;
      QString result;

   private:
      QList< VdpVideoSurface > slist;
      VdpVideoMixer mixer;
      VdpOutputSurface mixerSurface;
      int width, height;
      VDPAUContext *vc;
};



class VdpauWidget : public QWidget
{
   Q_OBJECT
   public:
      VdpauWidget( QString dataDirectory, QWidget *parent=0 );
      QString initVdpau();
      QString benchSurface();
      QString benchMPEG();
      QString benchMPEG720p();
      QString benchH264();
      QString benchH264720p();
      QString benchVC1();
      QString benchMPEG4();
      QString benchMixer();
      QString benchMT();

      QString getContext();
      QString getSummary();

   private:
      bool createMixer( int w, int h );
      void displayFrame( VdpVideoSurface surface, int w, int h, double ratio );
      void setDeinterlace( int deint );
      void setIvtc( int ivtc );
      void setSkipChroma( int on );
      void setHqScaling( int on );
      VDPAUContext *vc;
      QString dataDirectory;
      QString benchSurfaceResult, benchMPEGResult, benchMPEG720pResult, benchH264Result, benchH264720pResult, benchVC1Result, benchMPEG4Result, benchMixerResult, benchMTResult, benchSingleResult;

      VdpOutputSurface displaySurface, mixerSurface;
      VdpVideoMixer mixer;
      VdpPresentationQueueTarget queueTarget;
      VdpPresentationQueue queue;
      uint32_t mixerWidth, mixerHeight;
};
#endif // VDPAUWIDGET_H
