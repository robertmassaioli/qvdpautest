// kate: tab-indent on; indent-width 4; mixedindent off; indent-mode cstyle; remove-trailing-space on;
#include <QVBoxLayout>
#include "mainwidget.h"

#define LONGDELAY 5000
#define SHORTDELAY 2000

#define QVDPAUTEST_VERSION "qvdpautest 0.5.2\n"

MainWidget::MainWidget(QString dataDirectory) : QStackedWidget()
{
   te = new QTextEdit();
   te->setReadOnly( true );

   vw = new VdpauWidget(dataDirectory);

   lab = new QLabel( "For better results, make sure PowerMizer (if any) is set to Maximum Performance.\nIn the following tests, all images will be displayed in DECODING order." );
   lab->setAlignment( Qt::AlignCenter );
   lab->setFrameStyle( QFrame::Panel|QFrame::Sunken );
   lab->setFont( QFont("Helvetica", 16, QFont::Bold) );

   addWidget( te );
   addWidget( lab );
   addWidget( vw );

   setCurrentIndex( 1 );
   setFixedSize( 1024, 576 );

   step = 0;

   timer.setSingleShot( true );
   connect( &timer, SIGNAL(timeout()), this, SLOT(nextStep()) );
   timer.start( LONGDELAY );
}



QString MainWidget::getCPUModel()
{
   QFile file( "/proc/cpuinfo" );
   if ( !file.open(QIODevice::ReadOnly) ) {
      return "Unknown CPU";
   }

   QTextStream ts( &file );
   QString line;
   do {
      line = ts.readLine();
      if ( line.startsWith( "model name" ) ) {
         return line.split( ":" ).last().trimmed();
      }
   } while ( !line.isNull() );

   return "Unknown CPU";
}



QString MainWidget::getGPUModel()
{
   QFile file( "/var/log/Xorg.0.log" );
   if ( !file.open(QIODevice::ReadOnly) ) {
      return "Unknown GPU";
   }

   QTextStream ts( &file );
   QString line;
   do {
      line = ts.readLine();
      if ( line.contains( ": NVIDIA GPU" ) ) {
         int pos = line.indexOf("NVIDIA GPU");
         if ( pos!=-1 )
            return line.right( line.length()-pos ).trimmed();
      }
   } while ( !line.isNull() );

   return "Unknown GPU";
}



void MainWidget::nextStep()
{
   switch ( step ) {
      case -1: {
                  return;
               }
      case 0: {
                 QString s = vw->initVdpau();
                 if ( !s.isEmpty() ) {
                    lab->setText( QString("The following error occured: %1").arg(s) );
                    step = -1;
                    return;
                 }
                 QString res =  QVDPAUTEST_VERSION;
                 res += getCPUModel() + "\n";
                 res += getGPUModel() + "\n";
                 te->append( res );
                 printf( "%s\n", res.toAscii().data() );
                 res =  vw->getContext();
                 printf( "%s", res.toAscii().data() );
                 lab->setText( "VDPAU is now initialized.\nTesting PUT/GET bits..." );
                 timer.start( SHORTDELAY );
                 break;
              }
      case 1: {
                 QString res =  vw->benchSurface() + "\n";
                 printf( "%s", res.toAscii().data() );
                 lab->setText( "Testing MPEG 1080 decoding..." );
                 timer.start( SHORTDELAY );
                 break;
              }
      case 2: {
                 setCurrentIndex( 2 );
                 QString res =  vw->benchMPEG();
                 printf( "%s", res.toAscii().data() );
                 setCurrentIndex( 1 );
                 lab->setText( "Testing MPEG 720 decoding..." );
                 timer.start( SHORTDELAY );
                 break;
              }
      case 3: {
                 setCurrentIndex( 2 );
                 QString res =  vw->benchMPEG720p();
                 printf( "%s", res.toAscii().data() );
                 setCurrentIndex( 1 );
                 lab->setText( "Testing H264 1080 decoding..." );
                 timer.start( SHORTDELAY );
                 break;
              }
      case 4: {
                 setCurrentIndex( 2 );
                 QString res =  vw->benchH264();
                 printf( "%s", res.toAscii().data() );
                 setCurrentIndex( 1 );
                 lab->setText( "Testing H264 720 decoding..." );
                 timer.start( SHORTDELAY );
                 break;
              }
      case 5: {
                 setCurrentIndex( 2 );
                 QString res =  vw->benchH264720p();
                 printf( "%s", res.toAscii().data() );
                 setCurrentIndex( 1 );
                 lab->setText( "Testing VC1 decoding..." );
                 timer.start( SHORTDELAY );
                 break;
              }
      case 6: {
                 setCurrentIndex( 2 );
                 QString res =  vw->benchVC1();
                 printf( "%s", res.toAscii().data() );
                 setCurrentIndex( 1 );
                 lab->setText( "Testing MPEG4 decoding..." );
                 timer.start( SHORTDELAY );
                 break;
              }
      case 7: {
                 setCurrentIndex( 2 );
                 QString res =  vw->benchMPEG4() + "\n";
                 printf( "%s", res.toAscii().data() );
                 setCurrentIndex( 1 );
                 lab->setText( "Testing the video mixer, with various post processing settings..." );
                 timer.start( SHORTDELAY );
                 break;
              }
      case 8: {
                 QString res = vw->benchMixer() + "\n";
                 printf( "%s", res.toAscii().data() );
                 lab->setText( "Testing multithreading, decoding a MPEG 1080 stream in one thread\nand post processing a 1080 stream in a second thread..." );
                 timer.start( SHORTDELAY );
                 break;
              }
      case 9: {
                 QString res = vw->benchMT();
                 printf( "%s", res.toAscii().data() );
                 te->append( vw->getSummary() );
                 setCurrentIndex( 0 );
              }
   }

   ++step;
}
