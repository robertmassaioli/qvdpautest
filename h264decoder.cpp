// kate: tab-indent on; indent-width 4; mixedindent off; indent-mode cstyle; remove-trailing-space on;
#include <QFile>
#include <QDataStream>

#include "h264decoder.h"



H264Decoder::H264Decoder( VDPAUContext *v, QString filename )
{
	vc = v;
	decoder = VDP_INVALID_HANDLE;
	testFile = filename;
}



H264Decoder::~H264Decoder()
{
	while ( !frames.isEmpty() )
		delete frames.takeFirst();
	if ( decoder != VDP_INVALID_HANDLE )
		vc->vdp_decoder_destroy( decoder );
	int i;
	for ( i=0; i<NUMSURFACES; ++i ) {
		if ( surfaces[i]!=VDP_INVALID_HANDLE )
			vc->vdp_video_surface_destroy( surfaces[i] );
	}
}



bool H264Decoder::init()
{
	QFile file( testFile );
	if ( !file.open(QIODevice::ReadOnly) ) {
		fprintf( stderr, "%s", QString("H264Decoder: FATAL: Can't open %1 !!\n").arg(testFile).toLatin1().data() );
		return false;
	}
	
	QDataStream inData( &file );
	inData.readRawData( (char*)&width, 4 );
	inData.readRawData( (char*)&height, 4 );
	inData.readRawData( (char*)&ratio, 8 );
	inData.readRawData( (char*)&profile, 4 );
	int i;
	for ( i=0; i<FRAMESINSAMPLE; ++i ) {
		H264Frame *frame = new H264Frame();
		inData.readRawData( (char*)&frame->info, sizeof(frame->info) );
		inData.readRawData( (char*)&frame->dataLength, 4 );
		frame->data = new uint8_t[frame->dataLength];
		inData.readRawData( (char*)frame->data, frame->dataLength );
		frames.append( frame );
	}
	
	VdpStatus st = vc->vdp_decoder_create( vc->vdpDevice, profile, width, height, 16, &decoder );
	if ( st != VDP_STATUS_OK )
		return false;

	for ( i=0; i<NUMSURFACES; ++i ) {
		surfaces[ i ] = VDP_INVALID_HANDLE;
		st = vc->vdp_video_surface_create( vc->vdpDevice, VDP_CHROMA_TYPE_420, width, height, &surfaces[i] );
		if ( st != VDP_STATUS_OK ) {
			fprintf( stderr, "H264Decoder: FATAL: Can't create required surfaces!!\n" );
			return false;
		}
	}

	currentSurface = 0;
	currentFrame = 0;
	
	//fprintf( stderr, "H264Decoder: profile = %d\n", profile );
	
	return true;
}



VdpVideoSurface H264Decoder::getNextFrame()
{		
	H264Frame *frame = frames.at( currentFrame++ );
	
	VdpPictureInfoH264 info = frame->info;
	int i, k=currentFrame-1;
	for ( i=0; i<16; ++i ) {
		if ( info.referenceFrames[i].surface!=VDP_INVALID_HANDLE ) {
			info.referenceFrames[i].surface = refframes[info.referenceFrames[i].surface];
		}
	}
	
	VdpVideoSurface current = surfaces[currentSurface];
	
	VdpBitstreamBuffer vbit;
	vbit.struct_version = VDP_BITSTREAM_BUFFER_VERSION;
	vbit.bitstream = frame->data;
	vbit.bitstream_bytes = frame->dataLength;
	VdpStatus st = vc->vdp_decoder_render( decoder, current, (VdpPictureInfo*)&info, 1, &vbit );
	if ( st != VDP_STATUS_OK )
		fprintf( stderr, "H264Decoder: decoding failed!\n" );
	
	refframes[k] = current;
	++currentSurface;
	if ( currentSurface>=NUMSURFACES )
		currentSurface = 0;
	
	if ( currentFrame>=FRAMESINSAMPLE ) {
		currentFrame = 0;
		currentSurface = 0;
	}
		
	return current;
}
