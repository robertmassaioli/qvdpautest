// kate: tab-indent on; indent-width 4; mixedindent off; indent-mode cstyle; remove-trailing-space on;
#include <QFile>
#include <QDataStream>

#include "mpeg4decoder.h"

#ifdef VDP_DECODER_PROFILE_MPEG4_PART2_ASP



MPEG4Decoder::MPEG4Decoder( VDPAUContext *v )
{
	vc = v;
	decoder = VDP_INVALID_HANDLE;
}



MPEG4Decoder::~MPEG4Decoder()
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



bool MPEG4Decoder::init()
{
	QFile file( "mpeg4hd.dat" );
	if ( !file.open(QIODevice::ReadOnly) ) {
		fprintf( stderr, "MPEG4Decoder: FATAL: Can't open mpeg4hd.dat !!\n" );
		return false;
	}

	QDataStream inData( &file );
	inData.readRawData( (char*)&width, 4 );
	inData.readRawData( (char*)&height, 4 );
	inData.readRawData( (char*)&ratio, 8 );
	inData.readRawData( (char*)&profile, 4 );
	int i;
	for ( i=0; i<FRAMESINSAMPLE; ++i ) {
		MPEG4Frame *frame = new MPEG4Frame();
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
			fprintf( stderr, "MPEG4Decoder: FATAL: Can't create required surfaces!!\n" );
			return false;
		}
	}

	forwardRef = backwardRef = VDP_INVALID_HANDLE;
	currentSurface = surfaces[0];
	currentFrame = 0;

	return true;
}



VdpVideoSurface MPEG4Decoder::getNextFrame()
{
	MPEG4Frame *frame = frames.at( currentFrame++ );

	frame->info.backward_reference = VDP_INVALID_HANDLE;
	frame->info.forward_reference = VDP_INVALID_HANDLE;
	if ( frame->info.vop_coding_type==0 )
		frame->info.forward_reference = backwardRef;
	else if ( frame->info.vop_coding_type>=2 ) {
		frame->info.forward_reference = forwardRef;
		frame->info.backward_reference = backwardRef;
	}

	VdpBitstreamBuffer vbit;
	vbit.struct_version = VDP_BITSTREAM_BUFFER_VERSION;
	vbit.bitstream = frame->data;
	vbit.bitstream_bytes = frame->dataLength;
	VdpStatus st = vc->vdp_decoder_render( decoder, currentSurface, (VdpPictureInfo*)&frame->info, 1, &vbit );
	if ( st != VDP_STATUS_OK )
		fprintf( stderr, "MPEG4Decoder: decoding failed!\n" );

	if ( frame->info.vop_coding_type<2 ) {
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

#endif // VDP_DECODER_PROFILE_MPEG4_PART2_ASP
