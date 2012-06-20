// kate: tab-indent on; indent-width 4; mixedindent off; indent-mode cstyle; remove-trailing-space on;
#include <QFile>
#include <QDataStream>

#include "vc1decoder.h"



VC1Decoder::VC1Decoder( VDPAUContext *v, QString dataDirectory )
{
	vc = v;
	decoder = VDP_INVALID_HANDLE;

   dataFilename.append(dataDirectory);
   dataFilename.append("vc1hd.dat");
}



VC1Decoder::~VC1Decoder()
{
	while ( !frames.isEmpty() )
		delete frames.takeFirst();
	if ( decoder != VDP_INVALID_HANDLE )
		vc->vdp_decoder_destroy( decoder );
	int i;
	for ( i=0; i<3; ++i ) {
		if ( surfaces[i]!=VDP_INVALID_HANDLE )
			vc->vdp_video_surface_destroy( surfaces[i] );
	}
}



bool VC1Decoder::init()
{
	QFile file( dataFilename );
	if ( !file.open(QIODevice::ReadOnly) ) {
		fprintf( stderr, "%s", QString("VC1Decoder: FATAL: Can't open %1 !!\n").arg(dataFilename).toLatin1().data() );
		return false;
	}
	
	QDataStream inData( &file );
	inData.readRawData( (char*)&width, 4 );
	inData.readRawData( (char*)&height, 4 );
	inData.readRawData( (char*)&ratio, 8 );
	inData.readRawData( (char*)&profile, 4 );
	int i;
	for ( i=0; i<FRAMESINSAMPLE; ++i ) {
		VC1Frame *frame = new VC1Frame();
		inData.readRawData( (char*)&frame->info, sizeof(frame->info) );
		inData.readRawData( (char*)&frame->dataLength, 4 );
		frame->data = new uint8_t[frame->dataLength];
		inData.readRawData( (char*)frame->data, frame->dataLength );
		frames.append( frame );
	}
	
	VdpStatus st = vc->vdp_decoder_create( vc->vdpDevice, profile, width, height, 2, &decoder );
	if ( st != VDP_STATUS_OK )
		return false;

	for ( i=0; i<3; ++i ) {
		surfaces[ i ] = VDP_INVALID_HANDLE;
		st = vc->vdp_video_surface_create( vc->vdpDevice, VDP_CHROMA_TYPE_420, width, height, &surfaces[i] );
		if ( st != VDP_STATUS_OK ) {
			fprintf( stderr, "VC1Decoder: FATAL: Can't create required surfaces!!\n" );
			return false;
		}
	}
	
	forwardRef = backwardRef = VDP_INVALID_HANDLE;
	currentSurface = surfaces[0];
	currentFrame = 0;
	
	//fprintf( stderr, "VC1Decoder: profile = %d\n", profile );
	
	return true;
}



VdpVideoSurface VC1Decoder::getNextFrame()
{
	VC1Frame *frame = frames.at( currentFrame++ );
	
	frame->info.backward_reference = VDP_INVALID_HANDLE;
	frame->info.forward_reference = VDP_INVALID_HANDLE;
	if ( frame->info.picture_type==1 )
		frame->info.forward_reference = backwardRef;
	else if ( frame->info.picture_type>=3 ) {
		frame->info.forward_reference = forwardRef;
		frame->info.backward_reference = backwardRef;
	}
	
	VdpBitstreamBuffer vbit;
	vbit.struct_version = VDP_BITSTREAM_BUFFER_VERSION;
	vbit.bitstream = frame->data;
	vbit.bitstream_bytes = frame->dataLength;
	VdpStatus st = vc->vdp_decoder_render( decoder, currentSurface, (VdpPictureInfo*)&frame->info, 1, &vbit );
	if ( st != VDP_STATUS_OK )
		fprintf( stderr, "VC1Decoder: decoding failed!\n" );
		
	if ( frame->info.picture_type<3 ) {
		forwardRef = backwardRef;
		backwardRef = currentSurface;
	}
	VdpVideoSurface current = currentSurface;
	
	int i=0;
	currentSurface = surfaces[i];
	while ( (currentSurface==forwardRef || currentSurface==backwardRef) && i<3 )
		currentSurface = surfaces[++i];
		
	if ( currentFrame>=FRAMESINSAMPLE ) {
		forwardRef = backwardRef = VDP_INVALID_HANDLE;
		currentSurface = surfaces[0];
		currentFrame = 0;
	}
		
	return current;
}
