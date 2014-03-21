// kate: tab-indent on; indent-width 4; mixedindent off; indent-mode cstyle; remove-trailing-space on;
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include <QtGui>
#include <QX11Info>

#include "vdpauwidget.h"
#include "mpegdecoder.h"
#include "h264decoder.h"
#include "vc1decoder.h"
#include "mpeg4decoder.h"

#define FRAMESINLOOP 500
#define MIXERLOOP 5000 // ms
#define SHOWTIME 500000 // µs
#define GETPUTBITSLOOP 5000 // ms
#define SURFACEWIDTH 1920
#define SURFACEHEIGHT 1080

#define DEINT_BOB 1
#define DEINT_TEMPORAL 2
#define DEINT_TEMPORAL_SPATIAL 3

#define MIXER_LOOP 50

VdpauWidget::VdpauWidget( QString dataDir, QWidget *parent ) : QWidget( parent )
{
   QPalette palette = this->palette();
   palette.setColor(backgroundRole(), Qt::black);
   setPalette(palette);
   setAttribute(Qt::WA_OpaquePaintEvent, true);
   setAttribute(Qt::WA_PaintOnScreen, true);
   setFixedSize( 1024, 576 );

   vc = new VDPAUContext( x11Info().display(), x11Info().screen() );
   mixer = VDP_INVALID_HANDLE;
   mixerWidth = mixerHeight = 0;
   dataDirectory = dataDir;
}

QString VdpauWidget::initVdpau()
{
   QString res = vc->init();
   if ( !res.isEmpty() )
      return res;

   VdpStatus st = vc->vdp_output_surface_create( vc->vdpDevice, VDP_RGBA_FORMAT_B8G8R8A8, width(), height(), &displaySurface );
   if ( st != VDP_STATUS_OK ) {
      return "FATAL: Can't create display output surface !!\n";
   }

   if ( !createMixer( SURFACEWIDTH, SURFACEHEIGHT ) )
      return "FATAL: can't create mixer!\n";

   st = vc->vdp_presentation_queue_target_create_x11( vc->vdpDevice, winId(), &queueTarget );
   if ( st != VDP_STATUS_OK )
      return "FATAL: can't create queue target!";
   st = vc->vdp_presentation_queue_create( vc->vdpDevice, queueTarget, &queue );
   if ( st != VDP_STATUS_OK )
      return "FATAL: can't create display queue!";

   return "";
}

bool VdpauWidget::createMixer( int w, int h )
{
   if ( mixer!=VDP_INVALID_HANDLE )
      vc->vdp_video_mixer_destroy( mixer );

   int fcount = 5;
   VdpVideoMixerFeature mixer_features[6];
   mixer_features[0] = VDP_VIDEO_MIXER_FEATURE_NOISE_REDUCTION;
   mixer_features[1] = VDP_VIDEO_MIXER_FEATURE_SHARPNESS;
   mixer_features[2] = VDP_VIDEO_MIXER_FEATURE_DEINTERLACE_TEMPORAL;
   mixer_features[3] = VDP_VIDEO_MIXER_FEATURE_DEINTERLACE_TEMPORAL_SPATIAL;
   mixer_features[4] = VDP_VIDEO_MIXER_FEATURE_INVERSE_TELECINE;
#ifdef VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L1
   if ( vc->hqScalingSupported() ) {
      mixer_features[5] = VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L1;
      ++fcount;
   }
#endif
   VdpVideoMixerParameter params[] = { VDP_VIDEO_MIXER_PARAMETER_VIDEO_SURFACE_WIDTH, VDP_VIDEO_MIXER_PARAMETER_VIDEO_SURFACE_HEIGHT,
      VDP_VIDEO_MIXER_PARAMETER_CHROMA_TYPE, VDP_VIDEO_MIXER_PARAMETER_LAYERS };
   int num_layers = 3;
   VdpChromaType chroma = VDP_CHROMA_TYPE_420;
   void const *param_values[] = { &w, &h, &chroma, &num_layers };
   VdpStatus st = vc->vdp_video_mixer_create( vc->vdpDevice, fcount, mixer_features, 4, params, param_values, &mixer );
   if ( st != VDP_STATUS_OK ) {
      mixer = VDP_INVALID_HANDLE;
      return false;
   }

   setIvtc( 0 );
   setHqScaling( 0 );
   setSkipChroma( 0 );

   mixerWidth = w;
   mixerHeight = h;
   return true;
}



void VdpauWidget::setSkipChroma( int on )
{
   VdpVideoMixerAttribute attributes [] = { VDP_VIDEO_MIXER_ATTRIBUTE_SKIP_CHROMA_DEINTERLACE };
   void* values[] = { &on };
   VdpStatus st = vc->vdp_video_mixer_set_attribute_values( mixer, 1, attributes, values );
   if ( st != VDP_STATUS_OK )
      fprintf( stderr, "Can't set SKIP_CHROMA!\n" );
}



void VdpauWidget::setDeinterlace( int deint )
{
   VdpVideoMixerFeature features[] = { VDP_VIDEO_MIXER_FEATURE_DEINTERLACE_TEMPORAL, VDP_VIDEO_MIXER_FEATURE_DEINTERLACE_TEMPORAL_SPATIAL };
   VdpBool feature_enables[2];
   switch ( deint ) {
      case DEINT_BOB: feature_enables[0] = feature_enables[1] = 0; break;/* bob */
      case DEINT_TEMPORAL: feature_enables[0] = 1; feature_enables[1] = 0; break;/* temporal */
      case DEINT_TEMPORAL_SPATIAL: feature_enables[0] = feature_enables[1] = 1; break;/* temporal_spatial */
   }

   vc->vdp_video_mixer_set_feature_enables( mixer, 2, features, feature_enables );
}



void VdpauWidget::setIvtc( int ivtc )
{
   VdpVideoMixerFeature features[] = { VDP_VIDEO_MIXER_FEATURE_INVERSE_TELECINE };
   VdpBool feature_enables[] = { ivtc };

   vc->vdp_video_mixer_set_feature_enables( mixer, 1, features, feature_enables );
}




void VdpauWidget::setHqScaling( int on )
{
#ifdef VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L1	
   VdpVideoMixerFeature features[] = { VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L1 };
   VdpBool feature_enables[] = { on };

   vc->vdp_video_mixer_set_feature_enables( mixer, 1, features, feature_enables );
#endif	
}



void VdpauWidget::displayFrame( VdpVideoSurface surface, int w, int h, double ratio )
{
   Q_UNUSED( ratio );
   VdpRect vid_source = { 0, 0, w, h };
   VdpRect out_dest = { 0, 0, width(), height() };
   VdpStatus st = vc->vdp_video_mixer_render( mixer, VDP_INVALID_HANDLE, 0, VDP_VIDEO_MIXER_PICTURE_STRUCTURE_FRAME,
         0, 0, surface, 0, 0, &vid_source, displaySurface, &out_dest, &out_dest, 0, NULL );
   if ( st != VDP_STATUS_OK )
      printf( "displayFrame: vdp_video_mixer_render error : %s\n", vc->vdp_get_error_string( st ) );
   vc->vdp_presentation_queue_display( queue, displaySurface, 0, 0, 0 );
}



QString VdpauWidget::benchMixer()
{
   QString directoryName(dataDirectory);
   directoryName.append("mpghd.dat");
   MPEGDecoder *d = new MPEGDecoder( vc, directoryName );
   if ( !d->init() ) {
      delete d;
      return "Can't initialize MPEG decoder!";
   }

   if ( mixerWidth!=d->width || mixerHeight!=d->height )
      createMixer( d->width, d->height );

   QList< VdpVideoSurface > list = d->getOrderedFrames();

   VdpStatus st = vc->vdp_output_surface_create( vc->vdpDevice, VDP_RGBA_FORMAT_B8G8R8A8, d->width, d->height, &mixerSurface );
   if ( st != VDP_STATUS_OK ) {
      delete d;
      return "FATAL: Can't create mixer output surface !!\n";
   }

   int i, loop=0;
   VdpRect vid_source = { 0, 0, d->width, d->height };

   setSkipChroma( 0 );
   // weave
   setDeinterlace( DEINT_BOB );
   QTime t;
   int e;
   t.start();
   while ( t.elapsed() < MIXERLOOP ) {
      for ( i=1; i<NUMSURFACES-1; ++i ) {
         vc->vdp_video_mixer_render( mixer, VDP_INVALID_HANDLE, 0, VDP_VIDEO_MIXER_PICTURE_STRUCTURE_FRAME,
               0, 0, list.at(i), 0, 0, &vid_source, mixerSurface, &vid_source, &vid_source, 0, NULL );
      }
      ++loop;
   }
   e = t.elapsed();
   long n = (NUMSURFACES-2)*loop;
   benchMixerResult = QString("MIXER WEAVE (%1x%2): %3 frames/s\n").arg(d->width).arg(d->height).arg(n*1000/e);

   // bob
   loop = 0;
   t.start();
   while ( t.elapsed() < MIXERLOOP ) {
      for ( i=1; i<NUMSURFACES-1; ++i ) {
         st = vc->vdp_video_mixer_render( mixer, VDP_INVALID_HANDLE, 0, VDP_VIDEO_MIXER_PICTURE_STRUCTURE_BOTTOM_FIELD,
               0, 0, list.at(i), 0, 0, &vid_source, mixerSurface, &vid_source, &vid_source, 0, NULL );
         if ( st != VDP_STATUS_OK )
            fprintf( stderr, "vdp_video_mixer_render failed: %s\n", vc->vdp_get_error_string( st ) );
         st = vc->vdp_video_mixer_render( mixer, VDP_INVALID_HANDLE, 0, VDP_VIDEO_MIXER_PICTURE_STRUCTURE_TOP_FIELD,
               0, 0, list.at(i), 0, 0, &vid_source, mixerSurface, &vid_source, &vid_source, 0, NULL );
         if ( st != VDP_STATUS_OK )
            fprintf( stderr, "vdp_video_mixer_render failed: %s\n", vc->vdp_get_error_string( st ) );
      }
      loop += 2;
   }
   e = t.elapsed();
   n = (NUMSURFACES-2)*loop;
   benchMixerResult += QString("MIXER BOB (%1x%2): %3 fields/s\n").arg(d->width).arg(d->height).arg(n*1000/e);

   VdpVideoSurface past[2];
   VdpVideoSurface future[1];

   // temporal
   setDeinterlace( DEINT_TEMPORAL );
   loop = 0;
   t.start();
   while ( t.elapsed() < MIXERLOOP ) {
      for ( i=1; i<NUMSURFACES-1; ++i ) {
         past[1] = past[0] = list.at(i-1);
         future[0] = list.at(i);
         st = vc->vdp_video_mixer_render( mixer, VDP_INVALID_HANDLE, 0, VDP_VIDEO_MIXER_PICTURE_STRUCTURE_BOTTOM_FIELD,
               2, past, list.at(i), 1, future, &vid_source, mixerSurface, &vid_source, &vid_source, 0, NULL );
         if ( st != VDP_STATUS_OK )
            fprintf( stderr, "vdp_video_mixer_render failed: %s\n", vc->vdp_get_error_string( st ) );
         past[0] = list.at(i);
         future[0] = list.at(i+1);
         st = vc->vdp_video_mixer_render( mixer, VDP_INVALID_HANDLE, 0, VDP_VIDEO_MIXER_PICTURE_STRUCTURE_TOP_FIELD,
               2, past, list.at(i), 1, future, &vid_source, mixerSurface, &vid_source, &vid_source, 0, NULL );
         if ( st != VDP_STATUS_OK )
            fprintf( stderr, "vdp_video_mixer_render failed: %s\n", vc->vdp_get_error_string( st ) );
      }
      loop += 2;
   }
   e = t.elapsed();
   n = (NUMSURFACES-2)*loop;
   benchMixerResult += QString("MIXER TEMPORAL (%1x%2): %3 fields/s\n").arg(d->width).arg(d->height).arg(n*1000/e);

   // temporal + ivtc
   setIvtc( 1 );
   loop = 0;
   t.start();
   while ( t.elapsed() < MIXERLOOP ) {
      for ( i=1; i<NUMSURFACES-1; ++i ) {
         past[1] = past[0] = list.at(i-1);
         future[0] = list.at(i);
         st = vc->vdp_video_mixer_render( mixer, VDP_INVALID_HANDLE, 0, VDP_VIDEO_MIXER_PICTURE_STRUCTURE_BOTTOM_FIELD,
               2, past, list.at(i), 1, future, &vid_source, mixerSurface, &vid_source, &vid_source, 0, NULL );
         if ( st != VDP_STATUS_OK )
            fprintf( stderr, "vdp_video_mixer_render failed: %s\n", vc->vdp_get_error_string( st ) );
         past[0] = list.at(i);
         future[0] = list.at(i+1);
         st = vc->vdp_video_mixer_render( mixer, VDP_INVALID_HANDLE, 0, VDP_VIDEO_MIXER_PICTURE_STRUCTURE_TOP_FIELD,
               2, past, list.at(i), 1, future, &vid_source, mixerSurface, &vid_source, &vid_source, 0, NULL );
         if ( st != VDP_STATUS_OK )
            fprintf( stderr, "vdp_video_mixer_render failed: %s\n", vc->vdp_get_error_string( st ) );
      }
      loop += 2;
   }
   e = t.elapsed();
   n = (NUMSURFACES-2)*loop;
   benchMixerResult += QString("MIXER TEMPORAL + IVTC (%1x%2): %3 fields/s\n").arg(d->width).arg(d->height).arg(n*1000/e);
   setIvtc( 0 );

   // temporal + skip_chroma
   setSkipChroma( 1 );
   loop = 0;
   t.start();
   while ( t.elapsed() < MIXERLOOP ) {
      for ( i=1; i<NUMSURFACES-1; ++i ) {
         past[1] = past[0] = list.at(i-1);
         future[0] = list.at(i);
         st = vc->vdp_video_mixer_render( mixer, VDP_INVALID_HANDLE, 0, VDP_VIDEO_MIXER_PICTURE_STRUCTURE_BOTTOM_FIELD,
               2, past, list.at(i), 1, future, &vid_source, mixerSurface, &vid_source, &vid_source, 0, NULL );
         if ( st != VDP_STATUS_OK )
            fprintf( stderr, "vdp_video_mixer_render failed: %s\n", vc->vdp_get_error_string( st ) );
         past[0] = list.at(i);
         future[0] = list.at(i+1);
         st = vc->vdp_video_mixer_render( mixer, VDP_INVALID_HANDLE, 0, VDP_VIDEO_MIXER_PICTURE_STRUCTURE_TOP_FIELD,
               2, past, list.at(i), 1, future, &vid_source, mixerSurface, &vid_source, &vid_source, 0, NULL );
         if ( st != VDP_STATUS_OK )
            fprintf( stderr, "vdp_video_mixer_render failed: %s\n", vc->vdp_get_error_string( st ) );
      }
      loop += 2;
   }
   e = t.elapsed();
   n = (NUMSURFACES-2)*loop;
   benchMixerResult += QString("MIXER TEMPORAL + SKIP_CHROMA (%1x%2): %3 fields/s\n").arg(d->width).arg(d->height).arg(n*1000/e);
   setSkipChroma( 0 );

   // temporal_spatial
   setDeinterlace( DEINT_TEMPORAL_SPATIAL );
   loop = 0;
   t.start();
   while ( t.elapsed() < MIXERLOOP ) {
      for ( i=1; i<NUMSURFACES-1; ++i ) {
         past[1] = past[0] = list.at(i-1);
         future[0] = list.at(i);
         st = vc->vdp_video_mixer_render( mixer, VDP_INVALID_HANDLE, 0, VDP_VIDEO_MIXER_PICTURE_STRUCTURE_BOTTOM_FIELD,
               2, past, list.at(i), 1, future, &vid_source, mixerSurface, &vid_source, &vid_source, 0, NULL );
         if ( st != VDP_STATUS_OK )
            fprintf( stderr, "vdp_video_mixer_render failed: %s\n", vc->vdp_get_error_string( st ) );
         past[0] = list.at(i);
         future[0] = list.at(i+1);
         st = vc->vdp_video_mixer_render( mixer, VDP_INVALID_HANDLE, 0, VDP_VIDEO_MIXER_PICTURE_STRUCTURE_TOP_FIELD,
               2, past, list.at(i), 1, future, &vid_source, mixerSurface, &vid_source, &vid_source, 0, NULL );
         if ( st != VDP_STATUS_OK )
            fprintf( stderr, "vdp_video_mixer_render failed: %s\n", vc->vdp_get_error_string( st ) );
      }
      loop += 2;
   }
   e = t.elapsed();
   n = (NUMSURFACES-2)*loop;
   benchMixerResult += QString("MIXER TEMPORAL_SPATIAL (%1x%2): %3 fields/s\n").arg(d->width).arg(d->height).arg(n*1000/e);

   // temporal_spatial + ivtc
   setIvtc( 1 );
   loop = 0;
   t.start();
   while ( t.elapsed() < MIXERLOOP ) {
      for ( i=1; i<NUMSURFACES-1; ++i ) {
         past[1] = past[0] = list.at(i-1);
         future[0] = list.at(i);
         st = vc->vdp_video_mixer_render( mixer, VDP_INVALID_HANDLE, 0, VDP_VIDEO_MIXER_PICTURE_STRUCTURE_BOTTOM_FIELD,
               2, past, list.at(i), 1, future, &vid_source, mixerSurface, &vid_source, &vid_source, 0, NULL );
         if ( st != VDP_STATUS_OK )
            fprintf( stderr, "vdp_video_mixer_render failed: %s\n", vc->vdp_get_error_string( st ) );
         past[0] = list.at(i);
         future[0] = list.at(i+1);
         st = vc->vdp_video_mixer_render( mixer, VDP_INVALID_HANDLE, 0, VDP_VIDEO_MIXER_PICTURE_STRUCTURE_TOP_FIELD,
               2, past, list.at(i), 1, future, &vid_source, mixerSurface, &vid_source, &vid_source, 0, NULL );
         if ( st != VDP_STATUS_OK )
            fprintf( stderr, "vdp_video_mixer_render failed: %s\n", vc->vdp_get_error_string( st ) );
      }
      loop += 2;
   }
   e = t.elapsed();
   n = (NUMSURFACES-2)*loop;
   benchMixerResult += QString("MIXER TEMPORAL_SPATIAL + IVTC (%1x%2): %3 fields/s\n").arg(d->width).arg(d->height).arg(n*1000/e);
   setIvtc( 0 );

   // temporal_spatial + skip_chroma
   setSkipChroma( 1 );
   loop = 0;
   t.start();
   while ( t.elapsed() < MIXERLOOP ) {
      for ( i=1; i<NUMSURFACES-1; ++i ) {
         past[1] = past[0] = list.at(i-1);
         future[0] = list.at(i);
         st = vc->vdp_video_mixer_render( mixer, VDP_INVALID_HANDLE, 0, VDP_VIDEO_MIXER_PICTURE_STRUCTURE_BOTTOM_FIELD,
               2, past, list.at(i), 1, future, &vid_source, mixerSurface, &vid_source, &vid_source, 0, NULL );
         if ( st != VDP_STATUS_OK )
            fprintf( stderr, "vdp_video_mixer_render failed: %s\n", vc->vdp_get_error_string( st ) );
         past[0] = list.at(i);
         future[0] = list.at(i+1);
         st = vc->vdp_video_mixer_render( mixer, VDP_INVALID_HANDLE, 0, VDP_VIDEO_MIXER_PICTURE_STRUCTURE_TOP_FIELD,
               2, past, list.at(i), 1, future, &vid_source, mixerSurface, &vid_source, &vid_source, 0, NULL );
         if ( st != VDP_STATUS_OK )
            fprintf( stderr, "vdp_video_mixer_render failed: %s\n", vc->vdp_get_error_string( st ) );
      }
      loop += 2;
   }
   e = t.elapsed();
   n = (NUMSURFACES-2)*loop;
   benchMixerResult += QString("MIXER TEMPORAL_SPATIAL + SKIP_CHROMA (%1x%2): %3 fields/s\n").arg(d->width).arg(d->height).arg(n*1000/e);
   setSkipChroma( 0 );

   delete d;
   vc->vdp_output_surface_destroy( mixerSurface );

   // SD

   directoryName.clear();
   directoryName.append(dataDirectory);
   directoryName.append("mpgsd.dat");
   d = new MPEGDecoder( vc, directoryName );
   if ( !d->init() ) {
      delete d;
      return "Can't initialize MPEG decoder!";
   }

   if ( mixerWidth!=d->width || mixerHeight!=d->height )
      createMixer( d->width, d->height );

   list = d->getOrderedFrames();

   int sdwidth=1920, sdheight=1080;

   st = vc->vdp_output_surface_create( vc->vdpDevice, VDP_RGBA_FORMAT_B8G8R8A8, sdwidth, sdheight, &mixerSurface );
   if ( st != VDP_STATUS_OK ) {
      delete d;
      return "FATAL: Can't create mixer output surface !!\n";
   }

   vid_source.x1 = d->width;
   vid_source.y1 = d->height;

   VdpRect vid_dest = { 0, 0, sdwidth, sdheight };

   // temporal_spatial SD
   setDeinterlace( DEINT_TEMPORAL_SPATIAL );
   loop = 0;
   t.start();
   while ( t.elapsed() < MIXERLOOP ) {
      for ( i=1; i<NUMSURFACES-1; ++i ) {
         past[1] = past[0] = list.at(i-1);
         future[0] = list.at(i);
         st = vc->vdp_video_mixer_render( mixer, VDP_INVALID_HANDLE, 0, VDP_VIDEO_MIXER_PICTURE_STRUCTURE_TOP_FIELD,
               2, past, list.at(i), 1, future, &vid_source, mixerSurface, &vid_dest, &vid_dest, 0, NULL );
         if ( st != VDP_STATUS_OK )
            fprintf( stderr, "vdp_video_mixer_render failed: %s\n", vc->vdp_get_error_string( st ) );
         past[0] = list.at(i);
         future[0] = list.at(i+1);
         st = vc->vdp_video_mixer_render( mixer, VDP_INVALID_HANDLE, 0, VDP_VIDEO_MIXER_PICTURE_STRUCTURE_BOTTOM_FIELD,
               2, past, list.at(i), 1, future, &vid_source, mixerSurface, &vid_dest, &vid_dest, 0, NULL );
         if ( st != VDP_STATUS_OK )
            fprintf( stderr, "vdp_video_mixer_render failed: %s\n", vc->vdp_get_error_string( st ) );
      }
      loop+=2;
   }
   e = t.elapsed();
   n = (NUMSURFACES-2)*loop;
   benchMixerResult += QString("MIXER TEMPORAL_SPATIAL (%1x%2 video to 1920x1080 display): %3 fields/s\n").arg(d->width).arg(d->height).arg(n*1000/e);

   // temporal_spatial SD + hqScaling
   if ( vc->hqScalingSupported() ) {
      setHqScaling( 1 );
      loop = 0;
      t.start();
      while ( t.elapsed() < MIXERLOOP ) {
         for ( i=1; i<NUMSURFACES-1; ++i ) {
            past[1] = past[0] = list.at(i-1);
            future[0] = list.at(i);
            st = vc->vdp_video_mixer_render( mixer, VDP_INVALID_HANDLE, 0, VDP_VIDEO_MIXER_PICTURE_STRUCTURE_TOP_FIELD,
                  2, past, list.at(i), 1, future, &vid_source, mixerSurface, &vid_dest, &vid_dest, 0, NULL );
            if ( st != VDP_STATUS_OK )
               fprintf( stderr, "vdp_video_mixer_render failed: %s\n", vc->vdp_get_error_string( st ) );
            past[0] = list.at(i);
            future[0] = list.at(i+1);
            st = vc->vdp_video_mixer_render( mixer, VDP_INVALID_HANDLE, 0, VDP_VIDEO_MIXER_PICTURE_STRUCTURE_BOTTOM_FIELD,
                  2, past, list.at(i), 1, future, &vid_source, mixerSurface, &vid_dest, &vid_dest, 0, NULL );
            if ( st != VDP_STATUS_OK )
               fprintf( stderr, "vdp_video_mixer_render failed: %s\n", vc->vdp_get_error_string( st ) );
         }
         loop+=2;
      }
      e = t.elapsed();
      n = (NUMSURFACES-2)*loop;
      benchMixerResult += QString("MIXER TEMPORAL_SPATIAL + HQSCALING (%1x%2 video to 1920x1080 display): %3 fields/s\n").arg(d->width).arg(d->height).arg(n*1000/e);
      setHqScaling( 0 );
   }

   delete d;
   return benchMixerResult;
}



QString VdpauWidget::benchVC1()
{
   if ( !vc->isProfileSupported( VDPAUContext::ProfileVC1Main) )
      return "Profile unsupported.\n";

   VC1Decoder *d = new VC1Decoder( vc , dataDirectory);
   if ( !d->init() ) {
      delete d;
      return "Can't initialize VC1 decoder!\n";
   }

   if ( mixerWidth!=d->width || mixerHeight!=d->height )
      createMixer( d->width, d->height );

   int i;
   for ( i=0; i<FRAMESINSAMPLE; ++i ) {
      displayFrame( d->getNextFrame(), d->width, d->height, d->ratio );
      usleep( SHOWTIME );
   }

   QTime t;
   t.start();
   for ( i=0; i<FRAMESINLOOP; ++i ) {
      d->getNextFrame();
   }
   int e = t.elapsed();
   benchVC1Result = QString("VC1 DECODING (%1x%2): %3 frames/s\n").arg(d->width).arg(d->height).arg(FRAMESINLOOP*1000/e);

   delete d;
   return benchVC1Result;
}



QString VdpauWidget::benchMPEG4()
{
   if ( !vc->isProfileSupported( VDPAUContext::ProfileMPEG4ASP) )
      return "Profile unsupported.\n";
#ifdef VDP_DECODER_PROFILE_MPEG4_PART2_ASP
   MPEG4Decoder *d = new MPEG4Decoder( vc, dataDirectory );
   if ( !d->init() ) {
      delete d;
      return "Can't initialize MPEG4 decoder!\n";
   }

   if ( mixerWidth!=d->width || mixerHeight!=d->height )
      createMixer( d->width, d->height );

   int i;
   for ( i=0; i<FRAMESINSAMPLE; ++i ) {
      displayFrame( d->getNextFrame(), d->width, d->height, d->ratio );
      usleep( SHOWTIME );
   }

   QTime t;
   t.start();
   for ( i=0; i<FRAMESINLOOP; ++i ) {
      d->getNextFrame();
   }
   int e = t.elapsed();
   benchMPEG4Result = QString("MPEG4 DECODING (%1x%2): %3 frames/s\n").arg(d->width).arg(d->height).arg(FRAMESINLOOP*1000/e);

   delete d;
#endif // VDP_DECODER_PROFILE_MPEG4_PART2_ASP
   return benchMPEG4Result;
}



QString VdpauWidget::benchMPEG()
{
   if ( !vc->isProfileSupported( VDPAUContext::ProfileMPEG2Main) )
      return "Profile unsupported.\n";

   QString directoryName(dataDirectory);
   directoryName.append("mpghd.dat");
   MPEGDecoder *d = new MPEGDecoder( vc, directoryName );
   if ( !d->init() ) {
      delete d;
      return "Can't initialize MPEG decoder!\n";
   }

   if ( mixerWidth!=d->width || mixerHeight!=d->height )
      createMixer( d->width, d->height );

   int i;
   for ( i=0; i<FRAMESINSAMPLE; ++i ) {
      displayFrame( d->getNextFrame(), d->width, d->height, d->ratio );
      usleep( SHOWTIME );
   }

   QTime t;
   t.start();
   for ( i=0; i<FRAMESINLOOP; ++i ) {
      d->getNextFrame();
   }
   int e = t.elapsed();
   benchMPEGResult = QString("MPEG DECODING (%1x%2): %3 frames/s\n").arg(d->width).arg(d->height).arg(FRAMESINLOOP*1000/e);

   delete d;
   return benchMPEGResult;
}



QString VdpauWidget::benchMPEG720p()
{
   if ( !vc->isProfileSupported( VDPAUContext::ProfileMPEG2Main) )
      return "Profile unsupported.\n";

   QString directoryName(dataDirectory);
   directoryName.append("mpg720p.dat");
   MPEGDecoder *d = new MPEGDecoder( vc, directoryName );
   if ( !d->init() ) {
      delete d;
      return "Can't initialize MPEG decoder!\n";
   }

   if ( mixerWidth!=d->width || mixerHeight!=d->height )
      createMixer( d->width, d->height );

   int i;
   for ( i=0; i<FRAMESINSAMPLE; ++i ) {
      displayFrame( d->getNextFrame(), d->width, d->height, d->ratio );
      usleep( SHOWTIME );
   }

   QTime t;
   t.start();
   for ( i=0; i<FRAMESINLOOP; ++i ) {
      d->getNextFrame();
   }
   int e = t.elapsed();
   benchMPEG720pResult = QString("MPEG DECODING (%1x%2): %3 frames/s\n").arg(d->width).arg(d->height).arg(FRAMESINLOOP*1000/e);

   delete d;
   return benchMPEG720pResult;
}



QString VdpauWidget::benchH264()
{
   if ( !vc->isProfileSupported( VDPAUContext::ProfileH264Main) )
      return "Profile unsupported.\n";

   QString directoryName(dataDirectory);
   directoryName.append("h264hd.dat");
   H264Decoder *d = new H264Decoder( vc, directoryName );
   if ( !d->init() ) {
      delete d;
      return "Can't initialize H264 decoder!\n";
   }

   if ( mixerWidth!=d->width || mixerHeight!=d->height )
      createMixer( d->width, d->height );

   int i;
   for ( i=0; i<FRAMESINSAMPLE; ++i ) {
      displayFrame( d->getNextFrame(), d->width, d->height, d->ratio );
      usleep( SHOWTIME );
   }

   QTime t;
   t.start();
   for ( i=0; i<FRAMESINLOOP; ++i ) {
      d->getNextFrame();
   }
   int e = t.elapsed();
   benchH264Result = QString("H264 DECODING (%1x%2): %3 frames/s\n").arg(d->width).arg(d->height).arg(FRAMESINLOOP*1000/e);

   delete d;
   return benchH264Result;
}



QString VdpauWidget::benchH264720p()
{
   if ( !vc->isProfileSupported( VDPAUContext::ProfileH264High) )
      return "Profile unsupported.\n";

   QString directoryName(dataDirectory);
   directoryName.append("h264720p.dat");
   H264Decoder *d = new H264Decoder( vc, directoryName );
   if ( !d->init() ) {
      delete d;
      return "Can't initialize H264 decoder!\n";
   }

   if ( mixerWidth!=d->width || mixerHeight!=d->height )
      createMixer( d->width, d->height );

   int i;
   for ( i=0; i<FRAMESINSAMPLE; ++i ) {
      displayFrame( d->getNextFrame(), d->width, d->height, d->ratio );
      usleep( SHOWTIME );
   }

   QTime t;
   t.start();
   for ( i=0; i<FRAMESINLOOP; ++i ) {
      d->getNextFrame();
   }
   int e = t.elapsed();
   benchH264720pResult = QString("H264 DECODING (%1x%2): %3 frames/s\n").arg(d->width).arg(d->height).arg(FRAMESINLOOP*1000/e);

   delete d;
   return benchH264720pResult;
}



QString VdpauWidget::benchSurface()
{
   VdpVideoSurface surface;
   VdpStatus st = vc->vdp_video_surface_create( vc->vdpDevice, VDP_CHROMA_TYPE_422, SURFACEWIDTH, SURFACEHEIGHT, &surface );
   if ( st != VDP_STATUS_OK ) {
      return "FATAL: Can't create surface !!\n";
   }
   uint8_t *mem = new uint8_t[SURFACEWIDTH*SURFACEHEIGHT*2];
   if ( !mem ) {
      vc->vdp_video_surface_destroy( surface );
      return "FATAL: Can't alloc mem !!\n";
   }

   uint32_t pitch = 0;
   QTime t;
   int loop = 0;
   t.start();
   while ( t.elapsed() < GETPUTBITSLOOP ) {
      st = vc->vdp_video_surface_get_bits_y_cb_cr( surface, VDP_YCBCR_FORMAT_YUYV, (void* const*)&mem, &pitch );
      if ( st != VDP_STATUS_OK ) {
         vc->vdp_video_surface_destroy( surface );
         delete mem;
         return QString("FATAL: get_bits failed : %1!!\n").arg(vc->vdp_get_error_string(st));
      }
      ++loop;
   }
   int e = t.elapsed();
   float n = (SURFACEWIDTH*SURFACEHEIGHT*2.*loop)/1024/1024;
   benchSurfaceResult += QString("SURFACE GET BITS: %1 M/s\n").arg(n*1000/e);

   t.start();
   loop = 0;
   while ( t.elapsed() < GETPUTBITSLOOP ) {
      st = vc->vdp_video_surface_put_bits_y_cb_cr( surface, VDP_YCBCR_FORMAT_YUYV, (void* const*)&mem, &pitch );
      if ( st != VDP_STATUS_OK ) {
         vc->vdp_video_surface_destroy( surface );
         delete mem;
         return "FATAL: put_bits failed !!\n";
      }
      ++loop;
   }
   e = t.elapsed();
   n = (SURFACEWIDTH*SURFACEHEIGHT*2.*loop)/1024/1024;
   benchSurfaceResult += QString("SURFACE PUT BITS: %1 M/s\n").arg(n*1000/e);

   delete mem;
   vc->vdp_video_surface_destroy( surface );

   return benchSurfaceResult;
}



QString VdpauWidget::getSummary()
{
   QString res = vc->context + benchSurfaceResult + "\n" + benchMPEGResult + benchMPEG720pResult + benchH264Result + benchH264720pResult + benchVC1Result + benchMPEG4Result + "\n" + benchMixerResult + "\n" + benchMTResult; // + benchSingleResult;
   return res;
}



QString VdpauWidget::getContext()
{
   return vc->context;
}



// next 2 functions are the 2 threads.
// the first one (the main thread) runs the decoder
// the second one runs the mixer
QString VdpauWidget::benchMT()
{
   // init a mpeg decoder
   QString directoryName(dataDirectory);
   directoryName.append("mpghd.dat");
   MPEGDecoder *m = new MPEGDecoder( vc, directoryName );
   if ( !m->init() ) {
      delete m;
      return "Can't initialize MPEG decoder (1)!";
   }

   // create the rgba surface used by the mixer
   VdpStatus st = vc->vdp_output_surface_create( vc->vdpDevice, VDP_RGBA_FORMAT_B8G8R8A8, m->width, m->height, &mixerSurface );
   if ( st != VDP_STATUS_OK ) {
      delete m;
      return "FATAL: Can't create mixer output surface !!\n";
   }

   if ( mixerWidth!=m->width || mixerHeight!=m->height )
      createMixer( m->width, m->height );

   setDeinterlace( DEINT_TEMPORAL );

   // init the mixer thread
   // m->getOrderedFrames returns a list of 22 decoded surfaces in display order and destroys the decoder ...
   VdpauThread vt( vc, m->getOrderedFrames(), mixer, mixerSurface, m->width, m->height );

   // ... so we can create a new one here
   directoryName.clear();
   directoryName.append(dataDirectory);
   directoryName.append("mpghd.dat");
   MPEGDecoder *d = new MPEGDecoder( vc, directoryName );
   if ( !d->init( true ) ) {
      delete d;
      delete m;
      vc->vdp_output_surface_destroy( mixerSurface );
      return "Can't initialize MPEG decoder (2)!";
   }

   vt.running = true;
   // start the mixer thread
   vt.start();

   int loop=0;
   QTime t;
   t.start();
   // this is the decoder loop
   while ( t.elapsed() < MIXERLOOP ) {
      // decode next frame (25 frames in turn)
      d->getNextFrame();
      ++loop;
   }
   int e = t.elapsed();

   vt.running = false;
   // wait for the mixer thread to end
   vt.wait();

   benchMTResult = QString("MULTITHREADED MPEG DECODING (%1x%2): %3 frames/s\n").arg(d->width).arg(d->height).arg(loop*1000/e);
   benchMTResult += vt.result;

   delete d;
   delete m;
   vc->vdp_output_surface_destroy( mixerSurface );

   return benchMTResult;
}



void VdpauThread::run()
{
   VdpVideoSurface past[2];
   VdpVideoSurface future[1];
   VdpRect vid_source = { 0, 0, width, height };

   QTime t;
   t.start();
   int i=1, loop=0, e;
   while ( running ) {
      ++loop;
      past[1] = past[0] = slist.at(i-1);
      future[0] = slist.at(i);
      vc->vdp_video_mixer_render( mixer, VDP_INVALID_HANDLE, 0, VDP_VIDEO_MIXER_PICTURE_STRUCTURE_BOTTOM_FIELD,
            2, past, slist.at(i), 1, future, &vid_source, mixerSurface, &vid_source, &vid_source, 0, NULL );
      if ( ++i >= (NUMSURFACES-1) )
         i = 1;
   }
   e = t.elapsed();
   result = QString("MULTITHREADED MIXER TEMPORAL (%1x%2): %3 fields/s\n").arg(width).arg(height).arg(loop*1000/e);
}
