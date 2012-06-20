// kate: tab-indent on; indent-width 4; mixedindent off; indent-mode cstyle; remove-trailing-space on;
#include "vdpaucontext.h"



VDPAUContext::VDPAUContext( Display *d, int screen )
{
	vdpDisplay = d;
	vdpScreen = screen;
	profiles = 0;
	hqScaling = 0;
}



bool VDPAUContext::isProfileSupported( int p )
{
	return profiles & p;
}



void VDPAUContext::getDecoderCaps()
{
	queryDecoderCaps( VDP_DECODER_PROFILE_MPEG2_MAIN, "MPEG2_MAIN", ProfileMPEG2Main );
	queryDecoderCaps( VDP_DECODER_PROFILE_H264_MAIN, "H264_MAIN", ProfileH264Main );
	queryDecoderCaps( VDP_DECODER_PROFILE_H264_HIGH, "H264_HIGH", ProfileH264High );
	queryDecoderCaps( VDP_DECODER_PROFILE_VC1_MAIN, "VC1_MAIN", ProfileVC1Main );
#ifdef VDP_DECODER_PROFILE_MPEG4_PART2_ASP
	queryDecoderCaps( VDP_DECODER_PROFILE_MPEG4_PART2_ASP, "MPEG4_ASP", ProfileMPEG4ASP );
#endif
}



void VDPAUContext::queryDecoderCaps( VdpDecoderProfile p, QString title, Profiles prof )
{
	Q_UNUSED( title );
	VdpBool ok=false;
	uint32_t maxLevel, maxMB, maxWidth, maxHeight;
	vdp_decoder_query_capabilities( vdpDevice, p, &ok, &maxLevel, &maxMB, &maxWidth, &maxHeight );
	if ( ok )
		profiles |= prof;
}



bool VDPAUContext::getFunc( int func, void** p )
{
	VdpStatus st = vdp_get_proc_address( vdpDevice, func , p );
	if ( st != VDP_STATUS_OK )
		return false;
	return true;
}



QString VDPAUContext::init()
{
	QString err;
	
	vdp_get_proc_address = NULL;
	vdpDevice = VDP_INVALID_HANDLE;
	
	VdpStatus st = vdp_device_create_x11( vdpDisplay, vdpScreen, &vdpDevice, &vdp_get_proc_address );
	if ( st != VDP_STATUS_OK ) {
		err = "Can't create vdp device : ";
		if ( st == VDP_STATUS_NO_IMPLEMENTATION )
			err += "No vdpau implementation.";
		else
			err += "Unsupported GPU?";
		return err;
	}
	if ( vdpDevice==VDP_INVALID_HANDLE )
		return "Invalid VdpDevice handle !!";
	if ( !vdp_get_proc_address )
		return "vdp_get_proc_address is NULL !!";
		
	if ( !getFunc( VDP_FUNC_ID_GET_ERROR_STRING , (void**)&vdp_get_error_string ) )
		return "Can't get VDP_FUNC_ID_GET_ERROR_STRING address !!";
		
	if ( !getFunc( VDP_FUNC_ID_GET_API_VERSION , (void**)&vdp_get_api_version ) )
		return "Can't get VDP_FUNC_ID_GET_API_VERSION address !!";
		
	if ( !getFunc( VDP_FUNC_ID_GET_INFORMATION_STRING , (void**)&vdp_get_information_string ) )
		return "Can't get VDP_FUNC_ID_GET_INFORMATION_STRING address !!";
	
	if ( !getFunc( VDP_FUNC_ID_DEVICE_DESTROY , (void**)&vdp_device_destroy ) )
		return "Can't get VDP_FUNC_ID_DEVICE_DESTROY address !!";
	
	if ( !getFunc( VDP_FUNC_ID_GENERATE_CSC_MATRIX , (void**)&vdp_generate_csc_matrix ) )
		return "Can't get VDP_FUNC_ID_GENERATE_CSC_MATRIX address !!";
	
	if ( !getFunc( VDP_FUNC_ID_VIDEO_SURFACE_QUERY_CAPABILITIES , (void**)&vdp_video_surface_query_capabilities ) )
		return "Can't get VDP_FUNC_ID_VIDEO_SURFACE_QUERY_CAPABILITIES address !!";
	
	if ( !getFunc( VDP_FUNC_ID_VIDEO_SURFACE_QUERY_GET_PUT_BITS_Y_CB_CR_CAPABILITIES , (void**)&vdp_video_surface_query_get_put_bits_y_cb_cr_capabilities ) )
		return "Can't get VDP_FUNC_ID_VIDEO_SURFACE_QUERY_GET_PUT_BITS_Y_CB_CR_CAPABILITIES address !!";
		
	if ( !getFunc( VDP_FUNC_ID_VIDEO_SURFACE_CREATE , (void**)&vdp_video_surface_create ) )
		return "Can't get VDP_FUNC_ID_VIDEO_SURFACE_CREATE address !!";
		
	if ( !getFunc( VDP_FUNC_ID_VIDEO_SURFACE_DESTROY , (void**)&vdp_video_surface_destroy ) )
		return "Can't get VDP_FUNC_ID_VIDEO_SURFACE_DESTROY address !!";
		
	if ( !getFunc( VDP_FUNC_ID_VIDEO_SURFACE_GET_PARAMETERS , (void**)&vdp_video_surface_get_parameters ) )
		return "Can't get VDP_FUNC_ID_VIDEO_SURFACE_GET_PARAMETERS address !!";
		
	if ( !getFunc( VDP_FUNC_ID_VIDEO_SURFACE_GET_BITS_Y_CB_CR , (void**)&vdp_video_surface_get_bits_y_cb_cr ) )
		return "Can't get VDP_FUNC_ID_VIDEO_SURFACE_GET_BITS_Y_CB_CR address !!";
		
	if ( !getFunc( VDP_FUNC_ID_VIDEO_SURFACE_PUT_BITS_Y_CB_CR , (void**)&vdp_video_surface_put_bits_y_cb_cr ) )
		return "Can't get VDP_FUNC_ID_VIDEO_SURFACE_PUT_BITS_Y_CB_CR address !!";
		
	if ( !getFunc( VDP_FUNC_ID_OUTPUT_SURFACE_QUERY_CAPABILITIES , (void**)&vdp_output_surface_query_capabilities ) )
		return "Can't get VDP_FUNC_ID_OUTPUT_SURFACE_QUERY_CAPABILITIES address !!";
	
	if ( !getFunc( VDP_FUNC_ID_OUTPUT_SURFACE_QUERY_GET_PUT_BITS_NATIVE_CAPABILITIES , (void**)&vdp_output_surface_query_get_put_bits_native_capabilities ) )
		return "Can't get VDP_FUNC_ID_OUTPUT_SURFACE_QUERY_GET_PUT_BITS_NATIVE_CAPABILITIES address !!";
		
	if ( !getFunc( VDP_FUNC_ID_OUTPUT_SURFACE_QUERY_PUT_BITS_INDEXED_CAPABILITIES , (void**)&vdp_output_surface_query_put_bits_indexed_capabilities ) )
		return "Can't get VDP_FUNC_ID_OUTPUT_SURFACE_QUERY_PUT_BITS_INDEXED_CAPABILITIES address !!";
		
	if ( !getFunc( VDP_FUNC_ID_OUTPUT_SURFACE_QUERY_PUT_BITS_Y_CB_CR_CAPABILITIES , (void**)&vdp_output_surface_query_put_bits_y_cb_cr_capabilities ) )
		return "Can't get VDP_FUNC_ID_OUTPUT_SURFACE_QUERY_PUT_BITS_Y_CB_CR_CAPABILITIES address !!";
		
	if ( !getFunc( VDP_FUNC_ID_OUTPUT_SURFACE_CREATE , (void**)&vdp_output_surface_create ) )
		return "Can't get VDP_FUNC_ID_OUTPUT_SURFACE_CREATE address !!";
		
	if ( !getFunc( VDP_FUNC_ID_OUTPUT_SURFACE_DESTROY , (void**)&vdp_output_surface_destroy ) )
		return "Can't get VDP_FUNC_ID_OUTPUT_SURFACE_DESTROY address !!";
	
	if ( !getFunc( VDP_FUNC_ID_OUTPUT_SURFACE_GET_PARAMETERS , (void**)&vdp_output_surface_get_parameters ) )
		return "Can't get VDP_FUNC_ID_OUTPUT_SURFACE_GET_PARAMETERS address !!";
		
	if ( !getFunc( VDP_FUNC_ID_OUTPUT_SURFACE_GET_BITS_NATIVE , (void**)&vdp_output_surface_get_bits_native ) )
		return "Can't get VDP_FUNC_ID_OUTPUT_SURFACE_GET_BITS_NATIVE address !!";
		
	if ( !getFunc( VDP_FUNC_ID_OUTPUT_SURFACE_PUT_BITS_NATIVE , (void**)&vdp_output_surface_put_bits_native ) )
		return "Can't get VDP_FUNC_ID_OUTPUT_SURFACE_PUT_BITS_NATIVE address !!";
		
	if ( !getFunc( VDP_FUNC_ID_OUTPUT_SURFACE_PUT_BITS_INDEXED , (void**)&vdp_output_surface_put_bits_indexed ) )
		return "Can't get VDP_FUNC_ID_OUTPUT_SURFACE_PUT_BITS_INDEXED address !!";
		
	if ( !getFunc( VDP_FUNC_ID_OUTPUT_SURFACE_PUT_BITS_Y_CB_CR , (void**)&vdp_output_surface_put_bits_y_cb_cr ) )
		return "Can't get VDP_FUNC_ID_OUTPUT_SURFACE_PUT_BITS_Y_CB_CR address !!";
		
	if ( !getFunc( VDP_FUNC_ID_BITMAP_SURFACE_QUERY_CAPABILITIES , (void**)&vdp_bitmap_surface_query_capabilities ) )
		return "Can't get VDP_FUNC_ID_BITMAP_SURFACE_QUERY_CAPABILITIES address !!";
		
	if ( !getFunc( VDP_FUNC_ID_BITMAP_SURFACE_CREATE , (void**)&vdp_bitmap_surface_create ) )
		return "Can't get VDP_FUNC_ID_BITMAP_SURFACE_CREATE address !!";
		
	if ( !getFunc( VDP_FUNC_ID_BITMAP_SURFACE_DESTROY , (void**)&vdp_bitmap_surface_destroy ) )
		return "Can't get VDP_FUNC_ID_BITMAP_SURFACE_DESTROY address !!";
		
	if ( !getFunc( VDP_FUNC_ID_BITMAP_SURFACE_GET_PARAMETERS , (void**)&vdp_bitmap_surface_get_parameters ) )
		return "Can't get VDP_FUNC_ID_BITMAP_SURFACE_GET_PARAMETERS address !!";
		
	if ( !getFunc( VDP_FUNC_ID_BITMAP_SURFACE_PUT_BITS_NATIVE , (void**)&vdp_bitmap_surface_put_bits_native ) )
		return "Can't get VDP_FUNC_ID_BITMAP_SURFACE_PUT_BITS_NATIVE address !!";
		
	if ( !getFunc( VDP_FUNC_ID_OUTPUT_SURFACE_RENDER_OUTPUT_SURFACE , (void**)&vdp_output_surface_render_output_surface ) )
		return "Can't get VDP_FUNC_ID_OUTPUT_SURFACE_RENDER_OUTPUT_SURFACE address !!";
		
	if ( !getFunc( VDP_FUNC_ID_OUTPUT_SURFACE_RENDER_BITMAP_SURFACE , (void**)&vdp_output_surface_render_bitmap_surface ) )
		return "Can't get VDP_FUNC_ID_OUTPUT_SURFACE_RENDER_BITMAP_SURFACE address !!";
		
	if ( !getFunc( VDP_FUNC_ID_DECODER_QUERY_CAPABILITIES , (void**)&vdp_decoder_query_capabilities ) )
		return "Can't get VDP_FUNC_ID_DECODER_QUERY_CAPABILITIES address !!";
		
	if ( !getFunc( VDP_FUNC_ID_DECODER_CREATE , (void**)&vdp_decoder_create ) )
		return "Can't get VDP_FUNC_ID_DECODER_CREATE address !!";
		
	if ( !getFunc( VDP_FUNC_ID_DECODER_DESTROY , (void**)&vdp_decoder_destroy ) )
		return "Can't get VDP_FUNC_ID_DECODER_DESTROY address !!";
		
	if ( !getFunc( VDP_FUNC_ID_DECODER_GET_PARAMETERS , (void**)&vdp_decoder_get_parameters ) )
		return "Can't get VDP_FUNC_ID_DECODER_GET_PARAMETERS address !!";
		
	if ( !getFunc( VDP_FUNC_ID_DECODER_RENDER , (void**)&vdp_decoder_render ) )
		return "Can't get VDP_FUNC_ID_DECODER_RENDER address !!";
		
	if ( !getFunc( VDP_FUNC_ID_VIDEO_MIXER_QUERY_FEATURE_SUPPORT , (void**)&vdp_video_mixer_query_feature_support ) )
		return "Can't get VDP_FUNC_ID_VIDEO_MIXER_QUERY_FEATURE_SUPPORT address !!";
		
	if ( !getFunc( VDP_FUNC_ID_VIDEO_MIXER_QUERY_PARAMETER_SUPPORT , (void**)&vdp_video_mixer_query_parameter_support ) )
		return "Can't get VDP_FUNC_ID_VIDEO_MIXER_QUERY_PARAMETER_SUPPORT address !!";
		
	if ( !getFunc( VDP_FUNC_ID_VIDEO_MIXER_QUERY_ATTRIBUTE_SUPPORT , (void**)&vdp_video_mixer_query_attribute_support ) )
		return "Can't get VDP_FUNC_ID_VIDEO_MIXER_QUERY_ATTRIBUTE_SUPPORT address !!";
		
	if ( !getFunc( VDP_FUNC_ID_VIDEO_MIXER_QUERY_PARAMETER_VALUE_RANGE , (void**)&vdp_video_mixer_query_parameter_value_range ) )
		return "Can't get VDP_FUNC_ID_VIDEO_MIXER_QUERY_PARAMETER_VALUE_RANGE address !!";
		
	if ( !getFunc( VDP_FUNC_ID_VIDEO_MIXER_QUERY_ATTRIBUTE_VALUE_RANGE , (void**)&vdp_video_mixer_query_attribute_value_range ) )
		return "Can't get VDP_FUNC_ID_VIDEO_MIXER_QUERY_ATTRIBUTE_VALUE_RANGE address !!";
		
	if ( !getFunc( VDP_FUNC_ID_VIDEO_MIXER_CREATE , (void**)&vdp_video_mixer_create ) )
		return "Can't get VDP_FUNC_ID_VIDEO_MIXER_CREATE address !!";
		
	if ( !getFunc( VDP_FUNC_ID_VIDEO_MIXER_SET_FEATURE_ENABLES , (void**)&vdp_video_mixer_set_feature_enables ) )
		return "Can't get VDP_FUNC_ID_VIDEO_MIXER_SET_FEATURE_ENABLES address !!";
		
	if ( !getFunc( VDP_FUNC_ID_VIDEO_MIXER_SET_ATTRIBUTE_VALUES , (void**)&vdp_video_mixer_set_attribute_values ) )
		return "Can't get VDP_FUNC_ID_VIDEO_MIXER_SET_ATTRIBUTE_VALUES address !!";
		
	if ( !getFunc( VDP_FUNC_ID_VIDEO_MIXER_GET_FEATURE_SUPPORT , (void**)&vdp_video_mixer_get_feature_support ) )
		return "Can't get VDP_FUNC_ID_VIDEO_MIXER_GET_FEATURE_SUPPORT address !!";
		
	if ( !getFunc( VDP_FUNC_ID_VIDEO_MIXER_GET_FEATURE_ENABLES , (void**)&vdp_video_mixer_get_feature_enables ) )
		return "Can't get VDP_FUNC_ID_VIDEO_MIXER_GET_FEATURE_ENABLES address !!";
		
	if ( !getFunc( VDP_FUNC_ID_VIDEO_MIXER_GET_PARAMETER_VALUES , (void**)&vdp_video_mixer_get_parameter_values ) )
		return "Can't get VDP_FUNC_ID_VIDEO_MIXER_GET_PARAMETER_VALUES address !!";
		
	if ( !getFunc( VDP_FUNC_ID_VIDEO_MIXER_GET_ATTRIBUTE_VALUES , (void**)&vdp_video_mixer_get_attribute_values ) )
		return "Can't get VDP_FUNC_ID_VIDEO_MIXER_GET_ATTRIBUTE_VALUES address !!";
		
	if ( !getFunc( VDP_FUNC_ID_VIDEO_MIXER_DESTROY , (void**)&vdp_video_mixer_destroy ) )
		return "Can't get VDP_FUNC_ID_VIDEO_MIXER_DESTROY address !!";
		
	if ( !getFunc( VDP_FUNC_ID_VIDEO_MIXER_RENDER , (void**)&vdp_video_mixer_render ) )
		return "Can't get VDP_FUNC_ID_VIDEO_MIXER_RENDER address !!";
	
	if ( !getFunc( VDP_FUNC_ID_PRESENTATION_QUEUE_TARGET_CREATE_X11 , (void**)&vdp_presentation_queue_target_create_x11 ) )
		return "Can't get VDP_FUNC_ID_PRESENTATION_QUEUE_TARGET_CREATE_X11 address !!";
	
	if ( !getFunc( VDP_FUNC_ID_PRESENTATION_QUEUE_TARGET_DESTROY , (void**)&vdp_presentation_queue_target_destroy ) )
		return "Can't get VDP_FUNC_ID_PRESENTATION_QUEUE_TARGET_DESTROY address !!";
		
	if ( !getFunc( VDP_FUNC_ID_PRESENTATION_QUEUE_CREATE , (void**)&vdp_presentation_queue_create ) )
		return "Can't get VDP_FUNC_ID_PRESENTATION_QUEUE_CREATE address !!";
		
	if ( !getFunc( VDP_FUNC_ID_PRESENTATION_QUEUE_DESTROY , (void**)&vdp_presentation_queue_destroy ) )
		return "Can't get VDP_FUNC_ID_PRESENTATION_QUEUE_DESTROY address !!";
		
	if ( !getFunc( VDP_FUNC_ID_PRESENTATION_QUEUE_SET_BACKGROUND_COLOR , (void**)&vdp_presentation_queue_set_background_color ) )
		return "Can't get VDP_FUNC_ID_PRESENTATION_QUEUE_SET_BACKGROUND_COLOR address !!";
		
	if ( !getFunc( VDP_FUNC_ID_PRESENTATION_QUEUE_GET_BACKGROUND_COLOR , (void**)&vdp_presentation_queue_get_background_color ) )
		return "Can't get VDP_FUNC_ID_PRESENTATION_QUEUE_GET_BACKGROUND_COLOR address !!";
		
	if ( !getFunc( VDP_FUNC_ID_PRESENTATION_QUEUE_GET_TIME , (void**)&vdp_presentation_queue_get_time ) )
		return "Can't get VDP_FUNC_ID_PRESENTATION_QUEUE_GET_TIME address !!";
		
	if ( !getFunc( VDP_FUNC_ID_PRESENTATION_QUEUE_DISPLAY , (void**)&vdp_presentation_queue_display ) )
		return "Can't get VDP_FUNC_ID_PRESENTATION_QUEUE_DISPLAY address !!";
		
	if ( !getFunc( VDP_FUNC_ID_PRESENTATION_QUEUE_BLOCK_UNTIL_SURFACE_IDLE , (void**)&vdp_presentation_queue_block_until_surface_idle ) )
		return "Can't get VDP_FUNC_ID_PRESENTATION_QUEUE_BLOCK_UNTIL_SURFACE_IDLE address !!";
		
	if ( !getFunc( VDP_FUNC_ID_PRESENTATION_QUEUE_QUERY_SURFACE_STATUS , (void**)&vdp_presentation_queue_query_surface_status ) )
		return "Can't get VDP_FUNC_ID_PRESENTATION_QUEUE_QUERY_SURFACE_STATUS address !!";
		
	if ( !getFunc( VDP_FUNC_ID_PREEMPTION_CALLBACK_REGISTER , (void**)&vdp_preemption_callback_register ) )
		return "Can't get VDP_FUNC_ID_PREEMPTION_CALLBACK_REGISTER address !!";
	
	
	unsigned int tmp;
	vdp_get_api_version( &tmp );
	context.append( QString("VDPAU API version : %1\n").arg(tmp) );

	const char *s;
	st = vdp_get_information_string( &s );
	context.append( QString("VDPAU implementation : %1\n\n").arg(s) );
	
	getDecoderCaps();

#ifdef VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L1
	vdp_video_mixer_query_feature_support( vdpDevice, VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L1, &hqScaling );
#endif
	
	return "";
}
