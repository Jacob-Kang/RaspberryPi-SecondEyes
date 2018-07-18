/*
 * OpenMAX.cpp
 *
 *  Created on: 2015. 2. 5.
 *      Author:  Chulho Kang
 */

#include "../Include/OpenMAX.h"
#include <fcntl.h>
#include <unistd.h>

OpenMAX* OpenMAX::m_OpenMAX = NULL;

OpenMAX::OpenMAX()
    : r(OMX_ErrorNone),
      server_thread(0),
      is_end(0),
      output_written(0),
      omx_height(0),
      omx_width(0),
      omx_fps(0) {
  // TODO Auto-generated constructor stub
  memset(&eMirror, 0, sizeof(eMirror));
  rtsp_buffer = NULL;
  frame_buffer = NULL;
  rgb_frame_buffer = NULL;
  YUV_lookup_table();

}

OpenMAX::~OpenMAX() {
  // TODO Auto-generated destructor stub

  vcos_semaphore_delete(&ctx.handler_lock);
  if ((r = OMX_Deinit()) != OMX_ErrorNone) {
    omx_die(r, "OMX de-initalization failed");
  }
}

OpenMAX* OpenMAX::get_instance() {
  if (m_OpenMAX == NULL) {
    cout << "OpenMAX::Not create yet" << endl;
    m_OpenMAX = new OpenMAX;
  }
  return m_OpenMAX;
}

void OpenMAX::InitComponent() {

  /// camera init
  bcm_host_init();
  if ((r = OMX_Init()) != OMX_ErrorNone) {
    omx_die(r, "OMX initalization failed");
  }

  // Init context
  memset(&ctx, 0, sizeof(ctx));

  if (vcos_semaphore_create(&ctx.handler_lock, "handler_lock", 1)
      != VCOS_SUCCESS) {
    die("Failed to create handler lock semaphore");
  }

  // Init component handles
  memset(&ctx, 0, sizeof(callbacks));
  callbacks.EventHandler = event_handler;
  callbacks.FillBufferDone = fill_output_buffer_done_handler;

  memset(&ctx, 0, sizeof(camcallbacks));
  camcallbacks.EventHandler = event_handler;
  camcallbacks.FillBufferDone = fill_output_Camerabuffer_done_handler;

  init_component_handle("camera", &ctx.camera, &ctx, &camcallbacks);
  init_component_handle("video_encode", &ctx.encoder, &ctx, &callbacks);
  init_component_handle("null_sink", &ctx.null_sink, &ctx, &callbacks);

//	init_component_handle("image_encode", &ctx.imge_encoder, &ctx, &camcallbacks);

  say("Configuring camera...");
  say("Default port definition for camera input port 73");
  dump_port(ctx.camera, 73, OMX_TRUE);
  say("Default port definition for camera preview output port 70");
  dump_port(ctx.camera, 70, OMX_TRUE);
  say("Default port definition for camera video output port 71");
  dump_port(ctx.camera, 71, OMX_TRUE);
  say("Default port definition for camera video output port 72");
  dump_port(ctx.camera, 72, OMX_TRUE);

//	say("Default port definition for camera image output port 341");
//	dump_port(ctx.imge_encoder, 341, OMX_TRUE);
//	say("Default port definition for camera image output port 340");
//	dump_port(ctx.imge_encoder, 340, OMX_TRUE);

}
void OpenMAX::Load_camera_driver(int num_device) {
  // Request a callback to be made when OMX_IndexParamCameraDeviceNumber is
  // changed signaling that the camera device is ready for use.
  OMX_INIT_STRUCTURE(cbtype);
  cbtype.nPortIndex = OMX_ALL;
  cbtype.nIndex = OMX_IndexParamCameraDeviceNumber;
  cbtype.bEnable = OMX_TRUE;
  if ((r = OMX_SetConfig(ctx.camera, OMX_IndexConfigRequestCallback, &cbtype))
      != OMX_ErrorNone) {
    omx_die(
        r,
        "Failed to request camera device number parameter change for camera");
  }

  // Set device number, this triggers the callback configured just above
  OMX_INIT_STRUCTURE(device);
  device.nPortIndex = OMX_ALL;
  device.nU32 = num_device;
  if ((r = OMX_SetParameter(ctx.camera, OMX_IndexParamCameraDeviceNumber,
                            &device)) != OMX_ErrorNone) {
    omx_die(r, "Failed to set camera parameter device number");
  }
}
//PortNum							   Frame = 72, Encdoe = 71
//VIDEO_BITRATE                   10000000
//CAM_SHARPNESS                   0                       // -100 .. 100
//CAM_CONTRAST                    0                       // -100 .. 100
//CAM_BRIGHTNESS                  50                      // 0 .. 100
//CAM_SATURATION                  0                       // -100 .. 100
//CAM_EXPOSURE_VALUE_COMPENSTAION 0
//CAM_EXPOSURE_ISO_SENSITIVITY    100
void OpenMAX::ConfigOpenMAX(int _video_width, int _video_height, int _video_fps,
                            int _video_bitrate, int _cam_sharpness,
                            int _cam_contrast, int _cam_britness,
                            int _cam_saturation,
                            int _cam_exposure_value_compensation,
                            int _cam_exposure_iso_sensitivity) {
// Configure video format emitted by camera preview output port
  omx_width = _video_width;
  omx_height = _video_height;
  omx_fps = _video_fps;

  OMX_INIT_STRUCTURE(camera_portdef);
  camera_portdef.nPortIndex = 70;
  if ((r = OMX_GetParameter(ctx.camera, OMX_IndexParamPortDefinition,
                            &camera_portdef)) != OMX_ErrorNone) {
    omx_die(r,
            "Failed to get port definition for camera preview output port 70");
  }
  camera_portdef.format.video.nFrameWidth = omx_width;
  camera_portdef.format.video.nFrameHeight = omx_height;
  camera_portdef.format.video.xFramerate = omx_fps << 16;
  // Stolen from gstomxvideodec.c of gst-omx
  camera_portdef.format.video.nStride = (camera_portdef.format.video.nFrameWidth
      + camera_portdef.nBufferAlignment - 1)
      & (~(camera_portdef.nBufferAlignment - 1));
  camera_portdef.format.video.eColorFormat = OMX_COLOR_FormatYUV420PackedPlanar;
//	camera_portdef.format.video.eColorFormat = OMX_COLOR_Format32bitBGRA8888;
  if ((r = OMX_SetParameter(ctx.camera, OMX_IndexParamPortDefinition,
                            &camera_portdef)) != OMX_ErrorNone) {
    omx_die(r,
            "Failed to set port definition for camera preview output port 70");
  }
  // Configure video format emitted by camera video output port
  // Use configuration from camera preview output as basis for
  // camera video output configuration
//	OMX_INIT_STRUCTURE(camera_portdef);
//	camera_portdef.nPortIndex = 70;
//	if ((r = OMX_GetParameter(ctx.camera, OMX_IndexParamPortDefinition,	&camera_portdef)) != OMX_ErrorNone) {
//		omx_die(r,	"Failed to get port definition for camera preview output port 70");
//	}
  camera_portdef.nPortIndex = 71;
  if ((r = OMX_SetParameter(ctx.camera, OMX_IndexParamPortDefinition,
                            &camera_portdef)) != OMX_ErrorNone) {
    omx_die(r, "Failed to set port definition for camera video output port 71");
  }

  OMX_INIT_STRUCTURE(camera_portdef);
  camera_portdef.nPortIndex = 72;
  if ((r = OMX_GetParameter(ctx.camera, OMX_IndexParamPortDefinition,
                            &camera_portdef)) != OMX_ErrorNone) {
    omx_die(r, "Failed to get port definition for camera image output port 72");
  }

  camera_portdef.format.image.nFrameWidth = omx_width;
  camera_portdef.format.image.nFrameHeight = omx_height;
//		camera_portdef.format.video.eColorFormat = OMX_COLOR_Format32bitABGR8888;
//		camera_portdef.format.video.eColorFormat = OMX_COLOR_Format32bitBGRA8888;
  camera_portdef.format.image.eColorFormat = OMX_COLOR_FormatYUV420PackedPlanar;
  //	camera_portdef.format.image.eCompressionFormat = OMX_IMAGE_CodingUnused;
//		camera_portdef.format.image.eColorFormat = OMX_COLOR_Format24bitBGR888;
  //	camera_portdef.format.video.xFramerate = omx_fps << 16;
  camera_portdef.format.image.nStride = (camera_portdef.format.image.nFrameWidth
      + camera_portdef.nBufferAlignment - 1)
      & (~(camera_portdef.nBufferAlignment - 1));

  if ((r = OMX_SetParameter(ctx.camera, OMX_IndexParamPortDefinition,
                            &camera_portdef)) != OMX_ErrorNone) {
    omx_die(r, "Failed to set port definition for camera image output port 72");
  }

  // Configure frame rate
  OMX_INIT_STRUCTURE(framerate);
  framerate.nPortIndex = 70;
  framerate.xEncodeFramerate = camera_portdef.format.video.xFramerate;
  if ((r = OMX_SetConfig(ctx.camera, OMX_IndexConfigVideoFramerate, &framerate))
      != OMX_ErrorNone) {
    omx_die(
        r,
        "Failed to set framerate configuration for camera preview output port 70");
  }
  framerate.nPortIndex = 71;
  if ((r = OMX_SetConfig(ctx.camera, OMX_IndexConfigVideoFramerate, &framerate))
      != OMX_ErrorNone) {
    omx_die(
        r,
        "Failed to set framerate configuration for camera video output port 71");
  }
  // Configure sharpness
  OMX_INIT_STRUCTURE(sharpness);
  sharpness.nPortIndex = OMX_ALL;
  sharpness.nSharpness = _cam_sharpness;
  if ((r = OMX_SetConfig(ctx.camera, OMX_IndexConfigCommonSharpness, &sharpness))
      != OMX_ErrorNone) {
    omx_die(r, "Failed to set camera sharpness configuration");
  }
  // Configure contrast
  OMX_INIT_STRUCTURE(contrast);
  contrast.nPortIndex = OMX_ALL;
  contrast.nContrast = _cam_contrast;
  if ((r = OMX_SetConfig(ctx.camera, OMX_IndexConfigCommonContrast, &contrast))
      != OMX_ErrorNone) {
    omx_die(r, "Failed to set camera contrast configuration");
  }
  // Configure saturation
  OMX_INIT_STRUCTURE(saturation);
  saturation.nPortIndex = OMX_ALL;
  saturation.nSaturation = _cam_saturation;
  if ((r = OMX_SetConfig(ctx.camera, OMX_IndexConfigCommonSaturation,
                         &saturation)) != OMX_ErrorNone) {
    omx_die(r, "Failed to set camera saturation configuration");
  }
  // Configure brightness
  OMX_INIT_STRUCTURE(brightness);
  brightness.nPortIndex = OMX_ALL;
  brightness.nBrightness = _cam_britness;
  if ((r = OMX_SetConfig(ctx.camera, OMX_IndexConfigCommonBrightness,
                         &brightness)) != OMX_ErrorNone) {
    omx_die(r, "Failed to set camera brightness configuration");
  }
  // Configure exposure value
  OMX_INIT_STRUCTURE(exposure_value);
  exposure_value.nPortIndex = OMX_ALL;
  exposure_value.xEVCompensation = _cam_exposure_value_compensation;
  exposure_value.bAutoSensitivity = CAM_EXPOSURE_AUTO_SENSITIVITY;
  exposure_value.nSensitivity = _cam_exposure_iso_sensitivity;
  if ((r = OMX_SetConfig(ctx.camera, OMX_IndexConfigCommonExposureValue,
                         &exposure_value)) != OMX_ErrorNone) {
    omx_die(r, "Failed to set camera exposure value configuration");
  }
  //Exposure control

  OMX_INIT_STRUCTURE(exposure_control);
  exposure_control.nPortIndex = OMX_ALL;
  exposure_control.eExposureControl = CAM_EXPOSURE;
  if ((r = OMX_SetConfig(ctx.camera, OMX_IndexConfigCommonExposure,
                         &exposure_control))) {
    omx_die(r, "Failed to set camera Exposure control");
  }

  // Configure frame frame stabilisation
  OMX_INIT_STRUCTURE(frame_stabilisation_control);
  frame_stabilisation_control.nPortIndex = OMX_ALL;
  frame_stabilisation_control.bStab = CAM_FRAME_STABILISATION;
  if ((r = OMX_SetConfig(ctx.camera, OMX_IndexConfigCommonFrameStabilisation,
                         &frame_stabilisation_control)) != OMX_ErrorNone) {
    omx_die(
        r,
        "Failed to set camera frame frame stabilisation control configuration");
  }
  // Configure frame white balance control
  OMX_INIT_STRUCTURE(white_balance_control);
  white_balance_control.nPortIndex = OMX_ALL;
//	white_balance_control.eWhiteBalControl = CAM_WHITE_BALANCE_CONTROL;
  white_balance_control.eWhiteBalControl = OMX_WhiteBalControlAuto;
  if ((r = OMX_SetConfig(ctx.camera, OMX_IndexConfigCommonWhiteBalance,
                         &white_balance_control)) != OMX_ErrorNone) {
    omx_die(r,
            "Failed to set camera frame white balance control configuration");
  }

  //White balance gains (if white balance is set to off)
  if (!CAM_WHITE_BALANCE) {
    OMX_INIT_STRUCTURE(white_balance_gains_st);
    white_balance_gains_st.xGainR = (CAM_WHITE_BALANCE_RED_GAIN << 16) / 1000;
    white_balance_gains_st.xGainB = (CAM_WHITE_BALANCE_BLUE_GAIN << 16) / 1000;
    if ((r = OMX_SetConfig(ctx.camera, OMX_IndexConfigCustomAwbGains,
                           &white_balance_gains_st))) {
      omx_die(r, "Failed to set camera frame white balance gain");
    }
  }

  // Configure image filter
  OMX_INIT_STRUCTURE(image_filter);
  image_filter.nPortIndex = OMX_ALL;
  image_filter.eImageFilter = CAM_IMAGE_FILTER;
  if ((r = OMX_SetConfig(ctx.camera, OMX_IndexConfigCommonImageFilter,
                         &image_filter)) != OMX_ErrorNone) {
    omx_die(r, "Failed to set camera image filter configuration");
  }
  // Configure mirror
  eMirror = OMX_MirrorNone;
  if (CAM_FLIP_HORIZONTAL && !CAM_FLIP_VERTICAL) {
    eMirror = OMX_MirrorHorizontal;
  } else if (!CAM_FLIP_HORIZONTAL && CAM_FLIP_VERTICAL) {
    eMirror = OMX_MirrorVertical;
  } else if (CAM_FLIP_HORIZONTAL && CAM_FLIP_VERTICAL) {
    eMirror = OMX_MirrorBoth;
  }
  OMX_INIT_STRUCTURE(mirror);
  mirror.nPortIndex = 71;
  mirror.eMirror = eMirror;
  if ((r = OMX_SetConfig(ctx.camera, OMX_IndexConfigCommonMirror, &mirror))
      != OMX_ErrorNone) {
    omx_die(
        r,
        "Failed to set mirror configuration for camera video output port 71");
  }
  mirror.nPortIndex = 72;
  if ((r = OMX_SetConfig(ctx.camera, OMX_IndexConfigCommonMirror, &mirror))
      != OMX_ErrorNone) {
    omx_die(
        r,
        "Failed to set mirror configuration for camera video output port 71");
  }

  // Ensure camera is ready
  while (!ctx.camera_ready) {
    usleep(10000);
  }
  //////////////////////////////////////////////////////////

//	OMX_INIT_STRUCTURE (encoder_portdef);
//	encoder_portdef.nPortIndex = 341;
//	if ((r = OMX_GetParameter(ctx.imge_encoder, OMX_IndexParamPortDefinition,
//				&encoder_portdef)) != OMX_ErrorNone) {
//			omx_die(r, "Failed to get port definition for encoder output port 201");
//	}
//	encoder_portdef.format.image.nFrameWidth = omx_width;
//	encoder_portdef.format.image.nFrameHeight = omx_height;
//	encoder_portdef.format.image.eCompressionFormat = OMX_IMAGE_CodingJPEG;
//	encoder_portdef.format.image.eColorFormat = OMX_COLOR_FormatUnused;
//	if ((r = OMX_SetParameter(ctx.imge_encoder, OMX_IndexParamPortDefinition,	&encoder_portdef)) != OMX_ErrorNone) {
//			omx_die(r, "Failed to set port definition for encoder output port 201");
//		}
//
//
//	set_jpeg_settings();

  /////////////////////////////////////////////////////////

  say("Configuring encoder...");
  say("Default port definition for encoder input port 200");
  dump_port(ctx.encoder, 200, OMX_TRUE);
  say("Default port definition for encoder output port 201");
  dump_port(ctx.encoder, 201, OMX_TRUE);

  // Encoder input port definition is done automatically upon tunneling
  // Configure video format emitted by encoder output port
  OMX_INIT_STRUCTURE(encoder_portdef);
  encoder_portdef.nPortIndex = 201;
  if ((r = OMX_GetParameter(ctx.encoder, OMX_IndexParamPortDefinition,
                            &encoder_portdef)) != OMX_ErrorNone) {
    omx_die(r, "Failed to get port definition for encoder output port 201");
  }
  // Copy some of the encoder output port configuration
  // from camera output port

  camera_portdef.nPortIndex = 70;
  if ((r = OMX_GetParameter(ctx.camera, OMX_IndexParamPortDefinition,
                            &camera_portdef)) != OMX_ErrorNone) {
    omx_die(r,
            "Failed to get port definition for camera preview output port 70");
  }
  encoder_portdef.format.video.nFrameWidth = omx_width;
  encoder_portdef.format.video.nFrameHeight = omx_height;
  encoder_portdef.format.video.xFramerate = omx_fps << 16;
  encoder_portdef.format.video.nStride = camera_portdef.format.video.nStride;

  // Which one is effective, this or the configuration just below?
  encoder_portdef.format.video.nBitrate = _video_bitrate;
  if ((r = OMX_SetParameter(ctx.encoder, OMX_IndexParamPortDefinition,
                            &encoder_portdef)) != OMX_ErrorNone) {
    omx_die(r, "Failed to set port definition for encoder output port 201");
  }
  // Configure bitrate
  OMX_INIT_STRUCTURE(bitrate);
  bitrate.eControlRate = OMX_Video_ControlRateVariable;
  bitrate.nTargetBitrate = encoder_portdef.format.video.nBitrate;
  bitrate.nPortIndex = 201;
  if ((r = OMX_SetParameter(ctx.encoder, OMX_IndexParamVideoBitrate, &bitrate))
      != OMX_ErrorNone) {
    omx_die(r, "Failed to set bitrate for encoder output port 201");
  }
  // Configure format
  OMX_INIT_STRUCTURE(format);
  format.nPortIndex = 201;
  format.eCompressionFormat = OMX_VIDEO_CodingAVC;
  if ((r = OMX_SetParameter(ctx.encoder, OMX_IndexParamVideoPortFormat, &format))
      != OMX_ErrorNone) {
    omx_die(r, "Failed to set video format for encoder output port 201");
  }

  //AVC Profile
  OMX_INIT_STRUCTURE(avc_st);
  avc_st.nPortIndex = 201;
  if ((r = OMX_GetParameter(ctx.encoder, OMX_IndexParamVideoAvc, &avc_st))
      != OMX_ErrorNone) {
    omx_die(r, "Failed to get port definition for encoder output port 201");
  }
  avc_st.eProfile = OMX_VIDEO_AVCProfileBaseline;
  if ((r = OMX_SetParameter(ctx.encoder, OMX_IndexParamVideoAvc, &avc_st))) {
    omx_die(r, "Failed to get port definition for encoder output port 201");
  }

  say("Configuring null sink...");

  say("Default port definition for null sink input port 240");
  dump_port(ctx.null_sink, 240, OMX_TRUE);
}

#define JPEG_QUALITY 75 //1 .. 100
#define JPEG_EXIF_DISABLE OMX_FALSE
#define JPEG_IJG_ENABLE OMX_FALSE
#define JPEG_THUMBNAIL_ENABLE OMX_TRUE
#define JPEG_THUMBNAIL_WIDTH 64 //0 .. 1024
#define JPEG_THUMBNAIL_HEIGHT 48 //0 .. 1024
#define JPEG_PREVIEW OMX_FALSE

void OpenMAX::set_jpeg_settings() {
  printf("configuring setting");
//Quality
  OMX_IMAGE_PARAM_QFACTORTYPE quality;
  OMX_INIT_STRUCTURE(quality);
  quality.nPortIndex = 341;
  quality.nQFactor = JPEG_QUALITY;
  if ((r = OMX_SetParameter(ctx.imge_encoder, OMX_IndexParamQFactor, &quality))
      != OMX_ErrorNone) {
    omx_die(r, "Failed to set video format for encoder output port 201");
  }
//Disable EXIF tags
  OMX_CONFIG_BOOLEANTYPE exif;
  OMX_INIT_STRUCTURE(exif);
  exif.bEnabled = JPEG_EXIF_DISABLE;
  if ((r = OMX_SetParameter(ctx.imge_encoder, OMX_IndexParamBrcmDisableEXIF,
                            &exif))) {
    omx_die(r, "Failed to set video format for encoder output port 201");
  }
//Enable IJG table
  OMX_PARAM_IJGSCALINGTYPE ijg;
  OMX_INIT_STRUCTURE(ijg);
  ijg.nPortIndex = 341;
  ijg.bEnabled = JPEG_IJG_ENABLE;
  if ((r = OMX_SetParameter(ctx.imge_encoder,
                            OMX_IndexParamBrcmEnableIJGTableScaling, &ijg))) {
    omx_die(r, "Failed to set video format for encoder output port 201");
  }
//Thumbnail
  OMX_PARAM_BRCMTHUMBNAILTYPE thumbnail;
  OMX_INIT_STRUCTURE(thumbnail);
  thumbnail.bEnable = JPEG_THUMBNAIL_ENABLE;
  thumbnail.bUsePreview = JPEG_PREVIEW;
  thumbnail.nWidth = JPEG_THUMBNAIL_WIDTH;
  thumbnail.nHeight = JPEG_THUMBNAIL_HEIGHT;
  if ((r = OMX_SetParameter(ctx.imge_encoder, OMX_IndexParamBrcmThumbnail,
                            &thumbnail))) {
    omx_die(r, "Failed to set video format for encoder output port 201");
  }
//EXIF tags
//See firmware/documentation/ilcomponents/image_decode.html for valid keys
//	char key[] = "IFD0.Make";
//	char value[] = "Raspberry Pi";
//	int key_length = strlen(key);
//	int value_length = strlen(value);
//	struct {
////These two fields need to be together
//		OMX_CONFIG_METADATAITEMTYPE metadata_st;
//		char metadata_padding[value_length];
//	} item;
//	OMX_INIT_STRUCTURE(item.metadata_st);
//	item.metadata_st.nSize = sizeof(item);
//	item.metadata_st.eScopeMode = OMX_MetadataScopePortLevel;
//	item.metadata_st.nScopeSpecifier = 341;
//	item.metadata_st.eKeyCharset = OMX_MetadataCharsetASCII;
//	item.metadata_st.nKeySizeUsed = key_length;
//	memcpy(item.metadata_st.nKey, key, key_length);
//	item.metadata_st.eValueCharset = OMX_MetadataCharsetASCII;
//	item.metadata_st.nValueMaxSize = sizeof(item.metadata_padding);
//	item.metadata_st.nValueSizeUsed = value_length;
//	memcpy(item.metadata_st.nValue, value, value_length);
//	if ((r = OMX_SetConfig(ctx.imge_encoder, OMX_IndexConfigMetadataItem,
//			&item))) {
//		omx_die(r, "Failed to set video format for encoder output port 201");
//	}
}
void OpenMAX::InitOpenMAXPort() {
  // Null sink input port definition is done automatically upon tunneling

  // Tunnel camera preview output port and null sink input port
  say("Setting up tunnel from camera preview output port 70 to null sink input port 240...");
  if ((r = OMX_SetupTunnel(ctx.camera, 70, ctx.null_sink, 240))
      != OMX_ErrorNone) {
    omx_die(
        r,
        "Failed to setup tunnel between camera preview output port 70 and null sink input port 240");
  }

/////////////////////////////////////////////////////////////
//	say("Setting up tunnel from camera video output port 72 to encoder input port 340...");
//	 if ((r = OMX_SetupTunnel (ctx.camera, 72, ctx.imge_encoder, 340))!= OMX_ErrorNone){
//			 omx_die(r, "Failed to set video format for encoder output port 201");
//	}
//////////////////////////////

  // Tunnel camera video output port and encoder input port
  say("Setting up tunnel from camera video output port 71 to encoder input port 200...");
  if ((r = OMX_SetupTunnel(ctx.camera, 71, ctx.encoder, 200))
      != OMX_ErrorNone) {
    omx_die(
        r,
        "Failed to setup tunnel between camera video output port 71 and encoder input port 200");
  }

//	if ((r = OMX_SendCommand(ctx.imge_encoder, OMX_CommandStateSet, OMX_StateIdle, NULL)) != OMX_ErrorNone) {
//			omx_die(r, "Failed to switch state of the null sink component to idle");
//	}
//	block_until_state_changed(ctx.imge_encoder, OMX_StateIdle);
}

void OpenMAX::SwitchIdle() {
  say("Switching state of the camera component to idle...");
  if ((r = OMX_SendCommand(ctx.camera, OMX_CommandStateSet, OMX_StateIdle, NULL))
      != OMX_ErrorNone) {
    omx_die(r, "Failed to switch state of the camera component to idle");
  }
  block_until_state_changed(ctx.camera, OMX_StateIdle);
  say("Switching state of the encoder component to idle...");
  if ((r = OMX_SendCommand(ctx.encoder, OMX_CommandStateSet, OMX_StateIdle,
                           NULL)) != OMX_ErrorNone) {
    omx_die(r, "Failed to switch state of the encoder component to idle");
  }
  block_until_state_changed(ctx.encoder, OMX_StateIdle);
  say("Switching state of the null sink component to idle...");
  if ((r = OMX_SendCommand(ctx.null_sink, OMX_CommandStateSet, OMX_StateIdle,
                           NULL)) != OMX_ErrorNone) {
    omx_die(r, "Failed to switch state of the null sink component to idle");
  }
  block_until_state_changed(ctx.null_sink, OMX_StateIdle);
  // Switch components to idle state

}
void OpenMAX::EnablePort() {
  // Enable ports
  say("Enabling ports...");
  if ((r = OMX_SendCommand(ctx.camera, OMX_CommandPortEnable, 73, NULL))
      != OMX_ErrorNone) {
    omx_die(r, "Failed to enable camera input port 73");
  }
  block_until_port_changed(ctx.camera, 73, OMX_TRUE);
  if ((r = OMX_SendCommand(ctx.camera, OMX_CommandPortEnable, 70, NULL))
      != OMX_ErrorNone) {
    omx_die(r, "Failed to enable camera preview output port 70");
  }
  block_until_port_changed(ctx.camera, 70, OMX_TRUE);
  if ((r = OMX_SendCommand(ctx.camera, OMX_CommandPortEnable, 71, NULL))
      != OMX_ErrorNone) {
    omx_die(r, "Failed to enable camera video output port 71");
  }
  block_until_port_changed(ctx.camera, 71, OMX_TRUE);

  if ((r = OMX_SendCommand(ctx.camera, OMX_CommandPortEnable, 72, NULL))
      != OMX_ErrorNone) {
    omx_die(r, "Failed to enable camera video output port 72");
  }
  block_until_port_changed(ctx.camera, 72, OMX_TRUE);

  if ((r = OMX_SendCommand(ctx.encoder, OMX_CommandPortEnable, 200, NULL))
      != OMX_ErrorNone) {
    omx_die(r, "Failed to enable encoder input port 200");
  }
  block_until_port_changed(ctx.encoder, 200, OMX_TRUE);
  if ((r = OMX_SendCommand(ctx.encoder, OMX_CommandPortEnable, 201, NULL))
      != OMX_ErrorNone) {
    omx_die(r, "Failed to enable encoder output port 201");
  }
  block_until_port_changed(ctx.encoder, 201, OMX_TRUE);
  if ((r = OMX_SendCommand(ctx.null_sink, OMX_CommandPortEnable, 240, NULL))
      != OMX_ErrorNone) {
    omx_die(r, "Failed to enable null sink input port 240");
  }
  block_until_port_changed(ctx.null_sink, 240, OMX_TRUE);

  ////////////////////////////////////////////////////////////////
//	if ((r = OMX_SendCommand(ctx.imge_encoder, OMX_CommandPortEnable, 340, NULL)) != OMX_ErrorNone) {
//		omx_die(r, "Failed to enable null sink input port 340");
//	}
//	block_until_port_changed(ctx.imge_encoder, 340, OMX_TRUE);
//
//	if ((r = OMX_SendCommand(ctx.imge_encoder, OMX_CommandPortEnable, 341, NULL)) != OMX_ErrorNone) {
//			omx_die(r, "Failed to enable null sink input port 341");
//		}
//		block_until_port_changed(ctx.imge_encoder, 341, OMX_TRUE);
  ////////////////////////////////////////////////////
}

void OpenMAX::AllocateBuffer() {
  // Allocate camera input buffer and encoder output buffer,
  // buffers for tunneled ports are allocated internally by OMX
  say("Allocating buffers...");
  OMX_INIT_STRUCTURE(camera_portdef);
  camera_portdef.nPortIndex = 73;
  if ((r = OMX_GetParameter(ctx.camera, OMX_IndexParamPortDefinition,
                            &camera_portdef)) != OMX_ErrorNone) {
    omx_die(r, "Failed to get port definition for camera input port 73");
  }
  if ((r = OMX_AllocateBuffer(ctx.camera, &ctx.camera_ppBuffer_in, 73, NULL,
                              camera_portdef.nBufferSize)) != OMX_ErrorNone) {
    omx_die(r, "Failed to allocate buffer for camera input port 73");
  }

  ///////////////////////////////////////////////////////
  camera_portdef.nPortIndex = 72;
  if ((r = OMX_GetParameter(ctx.camera, OMX_IndexParamPortDefinition,
                            &camera_portdef)) != OMX_ErrorNone) {
    omx_die(r,
            "Failed to get port definition for camera v��deo output port 72");
  }
  if ((r = OMX_AllocateBuffer(ctx.camera, &ctx.camera_ppBuffer_out, 72, NULL,
                              camera_portdef.nBufferSize)) != OMX_ErrorNone) {
    omx_die(r, "Failed to allocate buffer for camera video output port 72");
  }
  ////////////////////////////////////////////////////////////////////////////////

  OMX_INIT_STRUCTURE(encoder_portdef);
  encoder_portdef.nPortIndex = 201;
  if ((r = OMX_GetParameter(ctx.encoder, OMX_IndexParamPortDefinition,
                            &encoder_portdef)) != OMX_ErrorNone) {
    omx_die(r, "Failed to get port definition for encoder output port 201");
  }
  if ((r = OMX_AllocateBuffer(ctx.encoder, &ctx.encoder_ppBuffer_out, 201, NULL,
                              encoder_portdef.nBufferSize)) != OMX_ErrorNone) {
    omx_die(r, "Failed to allocate buffer for encoder output port 201");
  }
  ////////////////////////////////////////////////
//	OMX_INIT_STRUCTURE(encoder_portdef);
//	encoder_portdef.nPortIndex = 341;
//	if ((r = OMX_GetParameter(ctx.imge_encoder, OMX_IndexParamPortDefinition, &encoder_portdef)) != OMX_ErrorNone) {
//		omx_die(r, "Failed to get port definition for encoder output port 341");
//	}
//	if ((r = OMX_AllocateBuffer(ctx.imge_encoder, &ctx.image_pp, 341,	NULL, encoder_portdef.nBufferSize)) != OMX_ErrorNone) {
//		omx_die(r, "Failed to allocate buffer for encoder output port 341");
//	}
  ///////////////////////////////////////////////
}

void OpenMAX::SwitchExecuting() {
  // Switch state of the components prior to starting
  // the video capture and encoding loop
  say("Switching state of the camera component to executing...");
  if ((r = OMX_SendCommand(ctx.camera, OMX_CommandStateSet, OMX_StateExecuting,
                           NULL)) != OMX_ErrorNone) {
    omx_die(r, "Failed to switch state of the camera component to executing");
  }
  block_until_state_changed(ctx.camera, OMX_StateExecuting);
  say("Switching state of the encoder component to executing...");
  if ((r = OMX_SendCommand(ctx.encoder, OMX_CommandStateSet, OMX_StateExecuting,
                           NULL)) != OMX_ErrorNone) {
    omx_die(r, "Failed to switch state of the encoder component to executing");
  }
  block_until_state_changed(ctx.encoder, OMX_StateExecuting);
  say("Switching state of the null sink component to executing...");
  if ((r = OMX_SendCommand(ctx.null_sink, OMX_CommandStateSet,
                           OMX_StateExecuting, NULL)) != OMX_ErrorNone) {
    omx_die(r,
            "Failed to switch state of the null sink component to executing");
  }
  block_until_state_changed(ctx.null_sink, OMX_StateExecuting);

  say("Configured port definition for camera input port 73");
  dump_port(ctx.camera, 73, OMX_FALSE);
  say("Configured port definition for camera video output port 72");
  dump_port(ctx.camera, 72, OMX_FALSE);
  say("Configured port definition for camera video output port 71");
  dump_port(ctx.camera, 71, OMX_FALSE);
  say("Configured port definition for camera preview output port 70");
  dump_port(ctx.camera, 70, OMX_FALSE);
  say("Configured port definition for encoder input port 200");
  dump_port(ctx.encoder, 200, OMX_FALSE);
  say("Configured port definition for encoder output port 201");
  dump_port(ctx.encoder, 201, OMX_FALSE);
  say("Configured port definition for null sink input port 240");
  dump_port(ctx.null_sink, 240, OMX_FALSE);
//	say("Configured port definition for null sink input port 341");
//	dump_port(ctx.imge_encoder, 341, OMX_FALSE);
  /////////////////////////////////////////////////////////////

//	if ((r = OMX_SendCommand(ctx.imge_encoder, OMX_CommandStateSet,OMX_StateExecuting, NULL)) != OMX_ErrorNone) {
//			omx_die(r,"Failed to switch state of the imge_encoder component to executing");
//	}
//	block_until_state_changed(ctx.imge_encoder, OMX_StateExecuting);
  /////////////////////////////////////////////////////////////

}

// 71 port = Video Encoder
// 72 port = Image Raw Data
void OpenMAX::SwitchCapturePort(int _capturePort, OMX_BOOL _portSet) {
  // Start capturing video with the camera
//	cout << "Capture Port #" << _capturePort << " : " << _portSet << endl;

  OMX_INIT_STRUCTURE(OMXcapture);
  OMXcapture.nPortIndex = _capturePort;
  OMXcapture.bEnabled = _portSet;
  if ((r = OMX_SetParameter(ctx.camera, OMX_IndexConfigPortCapturing,
                            &OMXcapture)) != OMX_ErrorNone) {
    omx_die(r, "Failed to switch on capture on camera Capture Port");
  }

}

//IplImage* OpenMAX::CaptureFrame(){
#define ROUND_UP_2(num) (((num)+1)&~1)
#define ROUND_UP_4(num) (((num)+3)&~3)

void OpenMAX::get_i420_frame_info(int width, int height, int buf_stride,
                                  int buf_slice_height, i420_frame_info *info) {
  info->p_stride[0] = ROUND_UP_4(width);
  info->p_stride[1] = ROUND_UP_4(ROUND_UP_2(width) / 2);
  info->p_stride[2] = info->p_stride[1];
  info->p_offset[0] = 0;
  info->p_offset[1] = info->p_stride[0] * ROUND_UP_2(height);
  info->p_offset[2] = info->p_offset[1]
      + info->p_stride[1] * (ROUND_UP_2(height) / 2);
  info->size = info->p_offset[2] + info->p_stride[2] * (ROUND_UP_2(height) / 2);
  info->width = width;
  info->height = height;
  info->buf_stride = buf_stride;
  info->buf_slice_height = buf_slice_height;
  info->buf_extra_padding =
      buf_slice_height >= 0 ?
          ((buf_slice_height && (height % buf_slice_height)) ?
              (buf_slice_height - (height % buf_slice_height)) : 0) :
          -1;
}
void OpenMAX::dump_frame_info(const char *message,
                              const i420_frame_info *info) {
  say("%s frame info:\n"
      "\tWidth:\t\t\t%d\n"
      "\tHeight:\t\t\t%d\n"
      "\tSize:\t\t\t%d\n"
      "\tBuffer stride:\t\t%d\n"
      "\tBuffer slice height:\t%d\n"
      "\tBuffer extra padding:\t%d\n"
      "\tPlane strides:\t\tY:%d U:%d V:%d\n"
      "\tPlane offsets:\t\tY:%d U:%d V:%d\n",
      message, info->width, info->height, info->size, info->buf_stride,
      info->buf_slice_height, info->buf_extra_padding, info->p_stride[0],
      info->p_stride[1], info->p_stride[2], info->p_offset[0],
      info->p_offset[1], info->p_offset[2]);
}
SEImage* OpenMAX::OMXCaptureFrame() {
  int need_next_buffer_to_be_filled = 1;
  int offset = 0;
//////////////////////////////////////////////////////////////////
  i420_frame_info frame_info, buf_info;
  get_i420_frame_info(camera_portdef.format.image.nFrameWidth,
                      camera_portdef.format.image.nFrameHeight,
                      camera_portdef.format.image.nStride,
                      camera_portdef.format.video.nSliceHeight, &frame_info);
  get_i420_frame_info(frame_info.buf_stride, frame_info.buf_slice_height, -1,
                      -1, &buf_info);
  // Buffer representing an I420 frame where to unpack
  // the fragmented Y, U, and V plane spans from the OMX buffers
  char *frame = (char*) calloc(1, frame_info.size);
  frame_buffer = (OMX_U8*) calloc(1, frame_info.size);
  if (frame == NULL) {
    die("Failed to allocate frame buffer");
  }
  // Some counters
  int frame_num = 1, buf_num = 0;
  size_t frame_bytes = 0, buf_size, buf_bytes_read = 0, buf_bytes_copied;
  int i;
  // I420 spec: U and V plane span size half of the size of the Y plane span size
  int max_spans_y = buf_info.height, max_spans_uv = max_spans_y / 2;
  int valid_spans_y, valid_spans_uv;
  // For unpack memory copy operation
  unsigned char *buf_start;
  int max_spans, valid_spans;
  int dst_offset, src_offset, span_size;
////////////////////////////////////////////////////////////////////
  while (1) {
    if (ctx.camera_output_buffer_available) {
//			say("Key frame boundry reached, exiting loop...");
      ctx.camera_output_buffer_available = 0;
//			say("Read from output buffer and wrote to output file %d/%d",ctx.camera_ppBuffer_out->nFilledLen, ctx.camera_ppBuffer_out->nAllocLen);
      if (ctx.buff_eos == true)
        break;
      buf_start = ctx.camera_ppBuffer_out->pBuffer
          + ctx.camera_ppBuffer_out->nOffset;
      // Size of the OMX buffer data;
      buf_size = ctx.camera_ppBuffer_out->nFilledLen;
      buf_bytes_read += buf_size;
      buf_bytes_copied = 0;
      // Detect the possibly non-full buffer in the last buffer of a frame
      valid_spans_y = max_spans_y
          - ((ctx.camera_ppBuffer_out->nFlags & OMX_BUFFERFLAG_ENDOFFRAME) ?
              frame_info.buf_extra_padding : 0);
      // I420 spec: U and V plane span size half of the size of the Y plane span size
      valid_spans_uv = valid_spans_y / 2;
      // Unpack Y, U, and V plane spans from the buffer to the I420 frame
      for (i = 0; i < 3; i++) {
        // Number of maximum and valid spans for this plane
        max_spans = (i == 0 ? max_spans_y : max_spans_uv);
        valid_spans = (i == 0 ? valid_spans_y : valid_spans_uv);
        dst_offset =
        // Start of the plane span in the I420 frame
            frame_info.p_offset[i] +
            // Plane spans copied from the previous buffers
                (buf_num * frame_info.p_stride[i] * max_spans);
        src_offset =
        // Start of the plane span in the buffer
            buf_info.p_offset[i];
        span_size =
        // Plane span size multiplied by the available spans in the buffer
            frame_info.p_stride[i] * valid_spans;
        memcpy(
        // Destination starts from the beginning of the frame and move forward by offset
            frame + dst_offset,
            // Source starts from the beginning of the OMX component buffer and move forward by offset
            buf_start + src_offset,
            // The final plane span size, possible padding at the end of
            // the plane span section in the buffer isn't included
            // since the size is based on the final frame plane span size
            span_size);
        buf_bytes_copied += span_size;
      }
      frame_bytes += buf_bytes_copied;
      buf_num++;
//				say("Read %d bytes from buffer %d of frame %d, copied %d bytes from %d Y spans and %d U/V spans available", buf_size, buf_num, frame_num, buf_bytes_copied, valid_spans_y, valid_spans_uv);
      if (ctx.camera_ppBuffer_out->nFlags & OMX_BUFFERFLAG_ENDOFFRAME) {
        // Dump the complete I420 frame
//					say("Captured frame %d, %d packed bytes read, %d bytes unpacked, writing %d unpacked frame bytes",frame_num, buf_bytes_read, frame_bytes, frame_info.size);
        if (frame_bytes != frame_info.size) {
          die("Frame bytes read %d doesn't match the frame size %d",
              frame_bytes, frame_info.size);
        }
//				output_written = fwrite(frame, 1, frame_info.size, ctx.fd_out);

        memcpy(frame_buffer + offset, frame, frame_info.size);
        offset += frame_info.size;
//				cout << "filled : " << offset << " / " << frame_info.size << endl;
        frame_num++;
        buf_num = 0;
        buf_bytes_read = 0;
        frame_bytes = 0;
        memset(frame, 0, frame_info.size);
      }
      // Flush buffer to output file
      need_next_buffer_to_be_filled = 1;
    }
    // Buffer flushed, request a new buffer to be filled by the encoder component
    if (need_next_buffer_to_be_filled) {
      need_next_buffer_to_be_filled = 0;
      ctx.camera_output_buffer_available = 0;
      if (ctx.camera_ppBuffer_out == NULL)
        cout << "buffer null" << endl;
//			cout<<"buffer need to be filled"<<endl;
      if ((r = OMX_FillThisBuffer(ctx.camera, ctx.camera_ppBuffer_out))
          != OMX_ErrorNone)
        omx_die(
            r,
            "Failed to request filling of the output buffer on encoder output port 201");
    }
  }

  ctx.buff_eos = false;
  ctx.camera_ppBuffer_out->nFlags = 0;

  // YUV420 -> RGB888
  memset(ctx.camera_ppBuffer_out->pBuffer, 0,
         ctx.camera_ppBuffer_out->nAllocLen);

  cap_frame.width = omx_width;
  cap_frame.height = omx_height;
  cap_frame.pData = frame_buffer;
  cap_frame.colorModel[0] = 'Y';
  cap_frame.colorModel[1] = 'U';
  cap_frame.colorModel[2] = 'V';
  cap_frame.colorModel[3] = '\0';
  cap_frame.nSize = sizeof(cap_frame);

  return &cap_frame;
}

void OpenMAX::OMXCaptureImage(char* filename, int quality) {
  int need_next_buffer_to_be_filled = 1;
  int offset = 0;
//////////////////////////////////////////////////////////////////
  i420_frame_info frame_info, buf_info;
  get_i420_frame_info(camera_portdef.format.image.nFrameWidth,
                      camera_portdef.format.image.nFrameHeight,
                      camera_portdef.format.image.nStride,
                      camera_portdef.format.video.nSliceHeight, &frame_info);
  get_i420_frame_info(frame_info.buf_stride, frame_info.buf_slice_height, -1,
                      -1, &buf_info);
  // Buffer representing an I420 frame where to unpack
  // the fragmented Y, U, and V plane spans from the OMX buffers
  char *frame = (char*) calloc(1, frame_info.size);
  frame_buffer = (OMX_U8*) calloc(1, frame_info.size);
  if (frame == NULL) {
    die("Failed to allocate frame buffer");
  }
  // Some counters
  int frame_num = 1, buf_num = 0;
  size_t frame_bytes = 0, buf_size, buf_bytes_read = 0, buf_bytes_copied;
  int i;
  // I420 spec: U and V plane span size half of the size of the Y plane span size
  int max_spans_y = buf_info.height, max_spans_uv = max_spans_y / 2;
  int valid_spans_y, valid_spans_uv;
  // For unpack memory copy operation
  unsigned char *buf_start;
  int max_spans, valid_spans;
  int dst_offset, src_offset, span_size;
////////////////////////////////////////////////////////////////////
  while (1) {
    if (ctx.camera_output_buffer_available) {
//			say("Key frame boundry reached, exiting loop...");
      ctx.camera_output_buffer_available = 0;
//			say("Read from output buffer and wrote to output file %d/%d",ctx.camera_ppBuffer_out->nFilledLen, ctx.camera_ppBuffer_out->nAllocLen);
      if (ctx.buff_eos == true)
        break;
      buf_start = ctx.camera_ppBuffer_out->pBuffer
          + ctx.camera_ppBuffer_out->nOffset;
      // Size of the OMX buffer data;
      buf_size = ctx.camera_ppBuffer_out->nFilledLen;
      buf_bytes_read += buf_size;
      buf_bytes_copied = 0;
      // Detect the possibly non-full buffer in the last buffer of a frame
      valid_spans_y = max_spans_y
          - ((ctx.camera_ppBuffer_out->nFlags & OMX_BUFFERFLAG_ENDOFFRAME) ?
              frame_info.buf_extra_padding : 0);
      // I420 spec: U and V plane span size half of the size of the Y plane span size
      valid_spans_uv = valid_spans_y / 2;
      // Unpack Y, U, and V plane spans from the buffer to the I420 frame
      for (i = 0; i < 3; i++) {
        // Number of maximum and valid spans for this plane
        max_spans = (i == 0 ? max_spans_y : max_spans_uv);
        valid_spans = (i == 0 ? valid_spans_y : valid_spans_uv);
        dst_offset =
        // Start of the plane span in the I420 frame
            frame_info.p_offset[i] +
            // Plane spans copied from the previous buffers
                (buf_num * frame_info.p_stride[i] * max_spans);
        src_offset =
        // Start of the plane span in the buffer
            buf_info.p_offset[i];
        span_size =
        // Plane span size multiplied by the available spans in the buffer
            frame_info.p_stride[i] * valid_spans;
        memcpy(
        // Destination starts from the beginning of the frame and move forward by offset
            frame + dst_offset,
            // Source starts from the beginning of the OMX component buffer and move forward by offset
            buf_start + src_offset,
            // The final plane span size, possible padding at the end of
            // the plane span section in the buffer isn't included
            // since the size is based on the final frame plane span size
            span_size);
        buf_bytes_copied += span_size;
      }
      frame_bytes += buf_bytes_copied;
      buf_num++;
//				say("Read %d bytes from buffer %d of frame %d, copied %d bytes from %d Y spans and %d U/V spans available", buf_size, buf_num, frame_num, buf_bytes_copied, valid_spans_y, valid_spans_uv);
      if (ctx.camera_ppBuffer_out->nFlags & OMX_BUFFERFLAG_ENDOFFRAME) {
        // Dump the complete I420 frame
//					say("Captured frame %d, %d packed bytes read, %d bytes unpacked, writing %d unpacked frame bytes",frame_num, buf_bytes_read, frame_bytes, frame_info.size);
        if (frame_bytes != frame_info.size) {
          die("Frame bytes read %d doesn't match the frame size %d",
              frame_bytes, frame_info.size);
        }
//				output_written = fwrite(frame, 1, frame_info.size, ctx.fd_out);

        memcpy(frame_buffer + offset, frame, frame_info.size);
        offset += frame_info.size;
//				cout << "filled : " << offset << " / " << frame_info.size << endl;
        frame_num++;
        buf_num = 0;
        buf_bytes_read = 0;
        frame_bytes = 0;
        memset(frame, 0, frame_info.size);
      }
      // Flush buffer to output file
      need_next_buffer_to_be_filled = 1;
    }
    // Buffer flushed, request a new buffer to be filled by the encoder component
    if (need_next_buffer_to_be_filled) {
      need_next_buffer_to_be_filled = 0;
      ctx.camera_output_buffer_available = 0;
      if (ctx.camera_ppBuffer_out == NULL)
        cout << "buffer null" << endl;
//			cout<<"buffer need to be filled"<<endl;
      if ((r = OMX_FillThisBuffer(ctx.camera, ctx.camera_ppBuffer_out))
          != OMX_ErrorNone)
        omx_die(
            r,
            "Failed to request filling of the output buffer on encoder output port 201");
    }
  }

  ctx.buff_eos = false;
  ctx.camera_ppBuffer_out->nFlags = 0;

  // YUV420 -> RGB888
  OMX_U8* rgb_frame_buffer = (OMX_U8*) malloc(omx_width * omx_height * 3);
  yuv420_to_rgb(frame_buffer, rgb_frame_buffer, omx_width, omx_height);
  memset(ctx.camera_ppBuffer_out->pBuffer, 0,
         ctx.camera_ppBuffer_out->nAllocLen);
  jpgm.write_JPEG_file(filename, 100, rgb_frame_buffer, omx_width, omx_height);

}

void OpenMAX::VideoEncode(const char* filename, int record_time) {
  // creaete video file descriptor
  ctx.fd_out = fopen(filename, "w");

  int quit_detected = 0, quit_in_keyframe = 0;
  int need_next_buffer_to_be_filled = 1;

  time_t start_time, end_time;
  time(&start_time);

  ctx.encoder_output_buffer_available = 0;

  while (1) {
    time(&end_time);
    //	 fill_output_buffer_done_handler() has marked that there's
    // a buffer for us to flush
    if (ctx.encoder_output_buffer_available) {
      // Print a message if the user wants to quit, but don't exit
      // the loop until we are certain that we have processed
      // a full frame till end of the frame, i.e. we're at the end
      // of the current key frame if processing one or until
      // the next key frame is detected. This way we should always
      // avoid corruption of the last encoded at the expense of
      // small delay in exiting.
      if (end_time - start_time == record_time) {
        quit_detected = 1;
        quit_in_keyframe = ctx.encoder_ppBuffer_out->nFlags
            & OMX_BUFFERFLAG_SYNCFRAME;
      }
      if (quit_detected
          && (quit_in_keyframe
              ^ (ctx.encoder_ppBuffer_out->nFlags & OMX_BUFFERFLAG_SYNCFRAME))) {
        say("Key frame boundry reached, exiting loop...");
        break;
      }

      // Flush buffer to output file
      output_written = fwrite(
          ctx.encoder_ppBuffer_out->pBuffer + ctx.encoder_ppBuffer_out->nOffset,
          1, ctx.encoder_ppBuffer_out->nFilledLen, ctx.fd_out);
      if (output_written != ctx.encoder_ppBuffer_out->nFilledLen) {
        die("Failed to write to output file: %s", strerror(errno));
      }
//			say("Read from output buffer and wrote to output file %d/%d",ctx.encoder_ppBuffer_out->nFilledLen, ctx.encoder_ppBuffer_out->nAllocLen);
      need_next_buffer_to_be_filled = 1;
    }
    // Buffer flushed, request a new buffer to be filled by the encoder component
    if (need_next_buffer_to_be_filled) {
      need_next_buffer_to_be_filled = 0;
      ctx.encoder_output_buffer_available = 0;
      if ((r = OMX_FillThisBuffer(ctx.encoder, ctx.encoder_ppBuffer_out))
          != OMX_ErrorNone) {
        omx_die(
            r,
            "Failed to request filling of the output buffer on encoder output port 201");
      }
    }
    // Would be better to use signaling here but hey this works too
    usleep(1000);
  }

  // Exit
  memset(&start_time, 0, sizeof(start_time));
  memset(&end_time, 0, sizeof(end_time));

  fclose(ctx.fd_out);
}

void OpenMAX::StartStreaming() {
  int quit_detected = 0, quit_in_keyframe = 0;
  int need_next_buffer_to_be_filled = 1;
  is_end = 0;
  rtsp_buffer = new PI_MEMORY_BUFFER(1000000 / VIDEO_FRAMERATE);

  // Create rtsp thread
  int ret = pthread_create(&server_thread, NULL, &startRtspServer,
                           (void*) rtsp_buffer);
  if (ret != 0) {
    fprintf(stderr, "error: pthread_create: %d\n", ret);
    exit(1);
  }

  ctx.encoder_output_buffer_available = 0;
  while (1) {
    if (ctx.encoder_output_buffer_available) {
      if (is_end == 1 && quit_detected == 0) {
        say("Exit signal detected(time out), waiting for next key frame boundry before exiting...");
        quit_detected = 1;
        quit_in_keyframe = ctx.encoder_ppBuffer_out->nFlags
            & OMX_BUFFERFLAG_SYNCFRAME;
      }
      if (quit_detected
          && (quit_in_keyframe
              ^ (ctx.encoder_ppBuffer_out->nFlags & OMX_BUFFERFLAG_SYNCFRAME))) {
        say("Key frame boundry reached, exiting loop...");
        break;
      }
//			say("Read from output buffer and wrote to output file %d/%d",ctx.encoder_ppBuffer_out->nFilledLen, ctx.encoder_ppBuffer_out->nAllocLen);
      rtsp_buffer->push_frame_data(ctx.encoder_ppBuffer_out->pBuffer,
                                   ctx.encoder_ppBuffer_out->nFilledLen);
      need_next_buffer_to_be_filled = 1;
    }

    if (need_next_buffer_to_be_filled) {

      need_next_buffer_to_be_filled = 0;
      ctx.encoder_output_buffer_available = 0;
      if ((r = OMX_FillThisBuffer(ctx.encoder, ctx.encoder_ppBuffer_out))
          != OMX_ErrorNone) {
        omx_die(
            r,
            "Failed to request filling of the output buffer on encoder output port 201");
      }
    }
    usleep(1000);
  }
  ctx.encoder_output_buffer_available = 0;
}

void OpenMAX::SwitchBufferFlush() {
  // Return the last full buffer back to the encoder component
  ctx.encoder_ppBuffer_out->nFlags = OMX_BUFFERFLAG_EOS;
  ctx.camera_ppBuffer_out->nFlags = OMX_BUFFERFLAG_EOS;

  if ((r = OMX_FillThisBuffer(ctx.encoder, ctx.encoder_ppBuffer_out))
      != OMX_ErrorNone)
    omx_die(
        r,
        "Failed to request filling of the output buffer on encoder output port 201");

  // Flush the buffers on each component
  if ((r = OMX_SendCommand(ctx.camera, OMX_CommandFlush, 73, NULL))
      != OMX_ErrorNone) {
    omx_die(r, "Failed to flush buffers of camera input port 73");
  }
  block_until_flushed(&ctx);
  if ((r = OMX_SendCommand(ctx.camera, OMX_CommandFlush, 70, NULL))
      != OMX_ErrorNone) {
    omx_die(r, "Failed to flush buffers of camera preview output port 70");
  }
  block_until_flushed(&ctx);
  if ((r = OMX_SendCommand(ctx.camera, OMX_CommandFlush, 72, NULL))
      != OMX_ErrorNone) {
    omx_die(r, "Failed to flush buffers of camera preview output port 72");
  }
  block_until_flushed(&ctx);
  if ((r = OMX_SendCommand(ctx.camera, OMX_CommandFlush, 71, NULL))
      != OMX_ErrorNone) {
    omx_die(r, "Failed to flush buffers of camera video output port 71");
  }
  block_until_flushed(&ctx);
  if ((r = OMX_SendCommand(ctx.encoder, OMX_CommandFlush, 200, NULL))
      != OMX_ErrorNone) {
    omx_die(r, "Failed to flush buffers of encoder input port 200");
  }
  block_until_flushed(&ctx);
  if ((r = OMX_SendCommand(ctx.encoder, OMX_CommandFlush, 201, NULL))
      != OMX_ErrorNone) {
    omx_die(r, "Failed to flush buffers of encoder output port 201");
  }
  block_until_flushed(&ctx);
  if ((r = OMX_SendCommand(ctx.null_sink, OMX_CommandFlush, 240, NULL))
      != OMX_ErrorNone) {
    omx_die(r, "Failed to flush buffers of null sink input port 240");
  }
  block_until_flushed(&ctx);

}

void OpenMAX::DisablePort() {

  // Disable all the ports
  if ((r = OMX_SendCommand(ctx.camera, OMX_CommandPortDisable, 73, NULL))
      != OMX_ErrorNone) {
    omx_die(r, "Failed to disable camera input port 73");
  }
  block_until_port_changed(ctx.camera, 73, OMX_FALSE);
  if ((r = OMX_SendCommand(ctx.camera, OMX_CommandPortDisable, 70, NULL))
      != OMX_ErrorNone) {
    omx_die(r, "Failed to disable camera preview output port 70");
  }
  block_until_port_changed(ctx.camera, 70, OMX_FALSE);
  if ((r = OMX_SendCommand(ctx.camera, OMX_CommandPortDisable, 71, NULL))
      != OMX_ErrorNone) {
    omx_die(r, "Failed to disable camera video output port 71");
  }

  block_until_port_changed(ctx.camera, 71, OMX_FALSE);
  if ((r = OMX_SendCommand(ctx.camera, OMX_CommandPortDisable, 72, NULL))
      != OMX_ErrorNone) {
    omx_die(r, "Failed to disable camera video output port 72");
  }

  block_until_port_changed(ctx.camera, 72, OMX_FALSE);
  if ((r = OMX_SendCommand(ctx.encoder, OMX_CommandPortDisable, 200, NULL))
      != OMX_ErrorNone) {
    omx_die(r, "Failed to disable encoder input port 200");
  }
  block_until_port_changed(ctx.encoder, 200, OMX_FALSE);
  if ((r = OMX_SendCommand(ctx.encoder, OMX_CommandPortDisable, 201, NULL))
      != OMX_ErrorNone) {
    omx_die(r, "Failed to disable encoder output port 201");
  }
  block_until_port_changed(ctx.encoder, 201, OMX_FALSE);
  if ((r = OMX_SendCommand(ctx.null_sink, OMX_CommandPortDisable, 240, NULL))
      != OMX_ErrorNone) {
    omx_die(r, "Failed to disable null sink input port 240");
  }
  block_until_port_changed(ctx.null_sink, 240, OMX_FALSE);
}

void OpenMAX::FreeBuffer() {
  // Free all the buffers
  if ((r = OMX_FreeBuffer(ctx.camera, 73, ctx.camera_ppBuffer_in))
      != OMX_ErrorNone) {
    omx_die(r, "Failed to free buffer for camera input port 73");
  }
  if ((r = OMX_FreeBuffer(ctx.camera, 72, ctx.camera_ppBuffer_out))
      != OMX_ErrorNone) {
    omx_die(r, "Failed to free buffer for camera input port 72");
  }
  if ((r = OMX_FreeBuffer(ctx.encoder, 201, ctx.encoder_ppBuffer_out))
      != OMX_ErrorNone) {
    omx_die(r, "Failed to free buffer for encoder output port 201");
  }
}

void OpenMAX::SwitchStateLoaded() {
  if ((r = OMX_SendCommand(ctx.camera, OMX_CommandStateSet, OMX_StateLoaded,
                           NULL)) != OMX_ErrorNone) {
    omx_die(r, "Failed to switch state of the encoder component to loaded");
  }
  cout << "loaded camera" << endl;
  //block_until_state_changed(ctx.camera, OMX_StateLoaded);
  if ((r = OMX_SendCommand(ctx.encoder, OMX_CommandStateSet, OMX_StateLoaded,
                           NULL)) != OMX_ErrorNone) {
    omx_die(r, "Failed to switch state of the encoder component to loaded");
  }
  cout << "loaded encoder" << endl;
  //block_until_state_changed(ctx.encoder, OMX_StateLoaded);
  if ((r = OMX_SendCommand(ctx.null_sink, OMX_CommandStateSet, OMX_StateLoaded,
                           NULL)) != OMX_ErrorNone) {
    omx_die(r, "Failed to switch state of the null sink component to loaded");
  }
  cout << "loaded null sink" << endl;
  //block_until_state_changed(ctx.null_sink, OMX_StateLoaded);

  // Free the component handles
  if ((r = OMX_FreeHandle(ctx.camera)) != OMX_ErrorNone) {
    omx_die(r, "Failed to free camera component handle");
  }
  cout << "free camera" << endl;
  if ((r = OMX_FreeHandle(ctx.encoder)) != OMX_ErrorNone) {
    omx_die(r, "Failed to free encoder component handle");
  }
  cout << "free encoder" << endl;
  if ((r = OMX_FreeHandle(ctx.null_sink)) != OMX_ErrorNone) {
    omx_die(r, "Failed to free null sink component handle");
  }
  cout << "free null sink" << endl;
}
void OpenMAX::DisableComponent() {
  OMX_INIT_STRUCTURE(OMXcapture);
  OMXcapture.nPortIndex = 71;
  OMXcapture.bEnabled = OMX_FALSE;
  if ((r = OMX_SetParameter(ctx.camera, OMX_IndexConfigPortCapturing,
                            &OMXcapture)) != OMX_ErrorNone) {
    omx_die(r, "Failed to switch off capture on camera video output port 71");
  }

  // Return the last full buffer back to the encoder component
  ctx.encoder_ppBuffer_out->nFlags = OMX_BUFFERFLAG_EOS;
  if ((r = OMX_FillThisBuffer(ctx.encoder, ctx.encoder_ppBuffer_out))
      != OMX_ErrorNone) {
    omx_die(
        r,
        "Failed to request filling of the output buffer on encoder output port 201");
  }

  // Flush the buffers on each component
  if ((r = OMX_SendCommand(ctx.camera, OMX_CommandFlush, 73, NULL))
      != OMX_ErrorNone) {
    omx_die(r, "Failed to flush buffers of camera input port 73");
  }
  block_until_flushed(&ctx);
  if ((r = OMX_SendCommand(ctx.camera, OMX_CommandFlush, 70, NULL))
      != OMX_ErrorNone) {
    omx_die(r, "Failed to flush buffers of camera preview output port 70");
  }
  block_until_flushed(&ctx);
  if ((r = OMX_SendCommand(ctx.camera, OMX_CommandFlush, 71, NULL))
      != OMX_ErrorNone) {
    omx_die(r, "Failed to flush buffers of camera video output port 71");
  }
  block_until_flushed(&ctx);
  if ((r = OMX_SendCommand(ctx.encoder, OMX_CommandFlush, 200, NULL))
      != OMX_ErrorNone) {
    omx_die(r, "Failed to flush buffers of encoder input port 200");
  }
  block_until_flushed(&ctx);
  if ((r = OMX_SendCommand(ctx.encoder, OMX_CommandFlush, 201, NULL))
      != OMX_ErrorNone) {
    omx_die(r, "Failed to flush buffers of encoder output port 201");
  }
  block_until_flushed(&ctx);
  if ((r = OMX_SendCommand(ctx.null_sink, OMX_CommandFlush, 240, NULL))
      != OMX_ErrorNone) {
    omx_die(r, "Failed to flush buffers of null sink input port 240");
  }
  block_until_flushed(&ctx);

  // Disable all the ports
  if ((r = OMX_SendCommand(ctx.camera, OMX_CommandPortDisable, 73, NULL))
      != OMX_ErrorNone) {
    omx_die(r, "Failed to disable camera input port 73");
  }
  block_until_port_changed(ctx.camera, 73, OMX_FALSE);
  if ((r = OMX_SendCommand(ctx.camera, OMX_CommandPortDisable, 70, NULL))
      != OMX_ErrorNone) {
    omx_die(r, "Failed to disable camera preview output port 70");
  }
  block_until_port_changed(ctx.camera, 70, OMX_FALSE);
  if ((r = OMX_SendCommand(ctx.camera, OMX_CommandPortDisable, 71, NULL))
      != OMX_ErrorNone) {
    omx_die(r, "Failed to disable camera video output port 71");
  }
  block_until_port_changed(ctx.camera, 71, OMX_FALSE);
  if ((r = OMX_SendCommand(ctx.encoder, OMX_CommandPortDisable, 200, NULL))
      != OMX_ErrorNone) {
    omx_die(r, "Failed to disable encoder input port 200");
  }
  block_until_port_changed(ctx.encoder, 200, OMX_FALSE);
  if ((r = OMX_SendCommand(ctx.encoder, OMX_CommandPortDisable, 201, NULL))
      != OMX_ErrorNone) {
    omx_die(r, "Failed to disable encoder output port 201");
  }
  block_until_port_changed(ctx.encoder, 201, OMX_FALSE);
  if ((r = OMX_SendCommand(ctx.null_sink, OMX_CommandPortDisable, 240, NULL))
      != OMX_ErrorNone) {
    omx_die(r, "Failed to disable null sink input port 240");
  }
  block_until_port_changed(ctx.null_sink, 240, OMX_FALSE);

  // Free all the buffers
  if ((r = OMX_FreeBuffer(ctx.camera, 73, ctx.camera_ppBuffer_in))
      != OMX_ErrorNone) {
    omx_die(r, "Failed to free buffer for camera input port 73");
  }
  if ((r = OMX_FreeBuffer(ctx.encoder, 201, ctx.encoder_ppBuffer_out))
      != OMX_ErrorNone) {
    omx_die(r, "Failed to free buffer for encoder output port 201");
  }

  // Transition all the components to idle and then to loaded states
  if ((r = OMX_SendCommand(ctx.camera, OMX_CommandStateSet, OMX_StateIdle, NULL))
      != OMX_ErrorNone) {
    omx_die(r, "Failed to switch state of the camera component to idle");
  }
  block_until_state_changed(ctx.camera, OMX_StateIdle);
  if ((r = OMX_SendCommand(ctx.encoder, OMX_CommandStateSet, OMX_StateIdle,
                           NULL)) != OMX_ErrorNone) {
    omx_die(r, "Failed to switch state of the encoder component to idle");
  }
  block_until_state_changed(ctx.encoder, OMX_StateIdle);
  if ((r = OMX_SendCommand(ctx.null_sink, OMX_CommandStateSet, OMX_StateIdle,
                           NULL)) != OMX_ErrorNone) {
    omx_die(r, "Failed to switch state of the null sink component to idle");
  }
  block_until_state_changed(ctx.null_sink, OMX_StateIdle);
  if ((r = OMX_SendCommand(ctx.camera, OMX_CommandStateSet, OMX_StateLoaded,
                           NULL)) != OMX_ErrorNone) {
    omx_die(r, "Failed to switch state of the camera component to loaded");
  }
  block_until_state_changed(ctx.camera, OMX_StateLoaded);
  if ((r = OMX_SendCommand(ctx.encoder, OMX_CommandStateSet, OMX_StateLoaded,
                           NULL)) != OMX_ErrorNone) {
    omx_die(r, "Failed to switch state of the encoder component to loaded");
  }
  block_until_state_changed(ctx.encoder, OMX_StateLoaded);
  if ((r = OMX_SendCommand(ctx.null_sink, OMX_CommandStateSet, OMX_StateLoaded,
                           NULL)) != OMX_ErrorNone) {
    omx_die(r, "Failed to switch state of the null sink component to loaded");
  }
  block_until_state_changed(ctx.null_sink, OMX_StateLoaded);

  // Free the component handles
  if ((r = OMX_FreeHandle(ctx.camera)) != OMX_ErrorNone) {
    omx_die(r, "Failed to free camera component handle");
  }
  if ((r = OMX_FreeHandle(ctx.encoder)) != OMX_ErrorNone) {
    omx_die(r, "Failed to free encoder component handle");
  }
  if ((r = OMX_FreeHandle(ctx.null_sink)) != OMX_ErrorNone) {
    omx_die(r, "Failed to free null sink component handle");
  }

//	vcos_semaphore_delete(&ctx.camera_lock);
//	if ((r = OMX_Deinit()) != OMX_ErrorNone) {
//		omx_die(r, "OMX de-initalization failed");
//	}

  endRtspServer();
  is_end = 0;
}
void OpenMAX::CloseComponet() {
  // Stop capturing video with the camera
  OMX_INIT_STRUCTURE(OMXcapture);
  OMXcapture.nPortIndex = 71;
  OMXcapture.bEnabled = OMX_FALSE;
  if ((r = OMX_SetParameter(ctx.camera, OMX_IndexConfigPortCapturing,
                            &OMXcapture)) != OMX_ErrorNone) {
    omx_die(r, "Failed to switch off capture on camera video output port 71");
  }

  // Return the last full buffer back to the encoder component
  ctx.encoder_ppBuffer_out->nFlags = OMX_BUFFERFLAG_EOS;
  if ((r = OMX_FillThisBuffer(ctx.encoder, ctx.encoder_ppBuffer_out))
      != OMX_ErrorNone) {
    omx_die(
        r,
        "Failed to request filling of the output buffer on encoder output port 201");
  }

  // Flush the buffers on each component
  if ((r = OMX_SendCommand(ctx.camera, OMX_CommandFlush, 73, NULL))
      != OMX_ErrorNone) {
    omx_die(r, "Failed to flush buffers of camera input port 73");
  }
  block_until_flushed(&ctx);
  if ((r = OMX_SendCommand(ctx.camera, OMX_CommandFlush, 70, NULL))
      != OMX_ErrorNone) {
    omx_die(r, "Failed to flush buffers of camera preview output port 70");
  }
  block_until_flushed(&ctx);
  if ((r = OMX_SendCommand(ctx.camera, OMX_CommandFlush, 71, NULL))
      != OMX_ErrorNone) {
    omx_die(r, "Failed to flush buffers of camera video output port 71");
  }
  block_until_flushed(&ctx);
  if ((r = OMX_SendCommand(ctx.encoder, OMX_CommandFlush, 200, NULL))
      != OMX_ErrorNone) {
    omx_die(r, "Failed to flush buffers of encoder input port 200");
  }
  block_until_flushed(&ctx);
  if ((r = OMX_SendCommand(ctx.encoder, OMX_CommandFlush, 201, NULL))
      != OMX_ErrorNone) {
    omx_die(r, "Failed to flush buffers of encoder output port 201");
  }
  block_until_flushed(&ctx);
  if ((r = OMX_SendCommand(ctx.null_sink, OMX_CommandFlush, 240, NULL))
      != OMX_ErrorNone) {
    omx_die(r, "Failed to flush buffers of null sink input port 240");
  }
  block_until_flushed(&ctx);

  // Disable all the ports
  if ((r = OMX_SendCommand(ctx.camera, OMX_CommandPortDisable, 73, NULL))
      != OMX_ErrorNone) {
    omx_die(r, "Failed to disable camera input port 73");
  }
  block_until_port_changed(ctx.camera, 73, OMX_FALSE);
  if ((r = OMX_SendCommand(ctx.camera, OMX_CommandPortDisable, 70, NULL))
      != OMX_ErrorNone) {
    omx_die(r, "Failed to disable camera preview output port 70");
  }
  block_until_port_changed(ctx.camera, 70, OMX_FALSE);
  if ((r = OMX_SendCommand(ctx.camera, OMX_CommandPortDisable, 71, NULL))
      != OMX_ErrorNone) {
    omx_die(r, "Failed to disable camera video output port 71");
  }
  block_until_port_changed(ctx.camera, 71, OMX_FALSE);
  if ((r = OMX_SendCommand(ctx.encoder, OMX_CommandPortDisable, 200, NULL))
      != OMX_ErrorNone) {
    omx_die(r, "Failed to disable encoder input port 200");
  }
  block_until_port_changed(ctx.encoder, 200, OMX_FALSE);
  if ((r = OMX_SendCommand(ctx.encoder, OMX_CommandPortDisable, 201, NULL))
      != OMX_ErrorNone) {
    omx_die(r, "Failed to disable encoder output port 201");
  }
  block_until_port_changed(ctx.encoder, 201, OMX_FALSE);
  if ((r = OMX_SendCommand(ctx.null_sink, OMX_CommandPortDisable, 240, NULL))
      != OMX_ErrorNone) {
    omx_die(r, "Failed to disable null sink input port 240");
  }
  block_until_port_changed(ctx.null_sink, 240, OMX_FALSE);

  // Free all the buffers
  if ((r = OMX_FreeBuffer(ctx.camera, 73, ctx.camera_ppBuffer_in))
      != OMX_ErrorNone) {
    omx_die(r, "Failed to free buffer for camera input port 73");
  }
  if ((r = OMX_FreeBuffer(ctx.encoder, 201, ctx.encoder_ppBuffer_out))
      != OMX_ErrorNone) {
    omx_die(r, "Failed to free buffer for encoder output port 201");
  }

  // Transition all the components to idle and then to loaded states
  if ((r = OMX_SendCommand(ctx.camera, OMX_CommandStateSet, OMX_StateIdle, NULL))
      != OMX_ErrorNone) {
    omx_die(r, "Failed to switch state of the camera component to idle");
  }
  block_until_state_changed(ctx.camera, OMX_StateIdle);
  if ((r = OMX_SendCommand(ctx.encoder, OMX_CommandStateSet, OMX_StateIdle,
                           NULL)) != OMX_ErrorNone) {
    omx_die(r, "Failed to switch state of the encoder component to idle");
  }
  block_until_state_changed(ctx.encoder, OMX_StateIdle);
  if ((r = OMX_SendCommand(ctx.null_sink, OMX_CommandStateSet, OMX_StateIdle,
                           NULL)) != OMX_ErrorNone) {
    omx_die(r, "Failed to switch state of the null sink component to idle");
  }
  block_until_state_changed(ctx.null_sink, OMX_StateIdle);
  if ((r = OMX_SendCommand(ctx.camera, OMX_CommandStateSet, OMX_StateLoaded,
                           NULL)) != OMX_ErrorNone) {
    omx_die(r, "Failed to switch state of the camera component to loaded");
  }
  block_until_state_changed(ctx.camera, OMX_StateLoaded);
  if ((r = OMX_SendCommand(ctx.encoder, OMX_CommandStateSet, OMX_StateLoaded,
                           NULL)) != OMX_ErrorNone) {
    omx_die(r, "Failed to switch state of the encoder component to loaded");
  }
  block_until_state_changed(ctx.encoder, OMX_StateLoaded);
  if ((r = OMX_SendCommand(ctx.null_sink, OMX_CommandStateSet, OMX_StateLoaded,
                           NULL)) != OMX_ErrorNone) {
    omx_die(r, "Failed to switch state of the null sink component to loaded");
  }
  block_until_state_changed(ctx.null_sink, OMX_StateLoaded);

  // Free the component handles
  if ((r = OMX_FreeHandle(ctx.camera)) != OMX_ErrorNone) {
    omx_die(r, "Failed to free camera component handle");
  }
  if ((r = OMX_FreeHandle(ctx.encoder)) != OMX_ErrorNone) {
    omx_die(r, "Failed to free encoder component handle");
  }
  if ((r = OMX_FreeHandle(ctx.null_sink)) != OMX_ErrorNone) {
    omx_die(r, "Failed to free null sink component handle");
  }

  // Exit
  fclose(ctx.fd_out);

  vcos_semaphore_delete(&ctx.handler_lock);
  if ((r = OMX_Deinit()) != OMX_ErrorNone) {
    omx_die(r, "OMX de-initalization failed");
  }
}
void OpenMAX::say(const char* message, ...) {
  va_list args;
  char str[1024];
  memset(str, 0, sizeof(str));
  va_start(args, message);
  vsnprintf(str, sizeof(str) - 1, message, args);
  va_end(args);
  size_t str_len = strnlen(str, sizeof(str));
  if (str[str_len - 1] != '\n') {
    str[str_len] = '\n';
  }
  fprintf(stderr, str);
}

void OpenMAX::die(const char* message, ...) {
  va_list args;
  char str[1024];
  memset(str, 0, sizeof(str));
  va_start(args, message);
  vsnprintf(str, sizeof(str), message, args);
  va_end(args);
  say(str);
  exit(1);
}

void OpenMAX::dump_event(OMX_HANDLETYPE hComponent, OMX_EVENTTYPE eEvent,
                         OMX_U32 nData1, OMX_U32 nData2) {
  const char *e;
  switch (eEvent) {
    case OMX_EventCmdComplete:
      e = "command complete";
      break;
    case OMX_EventError:
      e = "error";
      break;
    case OMX_EventParamOrConfigChanged:
      e = "parameter or configuration changed";
      break;
    case OMX_EventPortSettingsChanged:
      e = "port settings changed";
      break;
      /* That's all I've encountered during hacking so let's not bother with the rest... */
    default:
      e = "(no description)";
  }
  say("Received event 0x%08x %s, hComponent:0x%08x, nData1:0x%08x, nData2:0x%08x",
      eEvent, e, hComponent, nData1, nData2);
}

const char* OpenMAX::dump_compression_format(OMX_VIDEO_CODINGTYPE c) {
  char *f;
  switch (c) {
    case OMX_VIDEO_CodingUnused:
      return "not used";
    case OMX_VIDEO_CodingAutoDetect:
      return "autodetect";
    case OMX_VIDEO_CodingMPEG2:
      return "MPEG2";
    case OMX_VIDEO_CodingH263:
      return "H.263";
    case OMX_VIDEO_CodingMPEG4:
      return "MPEG4";
    case OMX_VIDEO_CodingWMV:
      return "Windows Media Video";
    case OMX_VIDEO_CodingRV:
      return "RealVideo";
    case OMX_VIDEO_CodingAVC:
      return "H.264/AVC";
    case OMX_VIDEO_CodingMJPEG:
      return "Motion JPEG";
    case OMX_VIDEO_CodingVP6:
      return "VP6";
    case OMX_VIDEO_CodingVP7:
      return "VP7";
    case OMX_VIDEO_CodingVP8:
      return "VP8";
    case OMX_VIDEO_CodingYUV:
      return "Raw YUV video";
    case OMX_VIDEO_CodingSorenson:
      return "Sorenson";
    case OMX_VIDEO_CodingTheora:
      return "OGG Theora";
    case OMX_VIDEO_CodingMVC:
      return "H.264/MVC";

    default:
      f = (char*) calloc(23, sizeof(char));
      if (f == NULL) {
        die("Failed to allocate memory");
      }
      snprintf(f, 23 * sizeof(char) - 1, "format type 0x%08x", c);
      return f;
  }
}
const char* OpenMAX::dump_color_format(OMX_COLOR_FORMATTYPE c) {
  const char *f;
  switch (c) {
    case OMX_COLOR_FormatUnused:
      return "OMX_COLOR_FormatUnused: not used";
    case OMX_COLOR_FormatMonochrome:
      return "OMX_COLOR_FormatMonochrome";
    case OMX_COLOR_Format8bitRGB332:
      return "OMX_COLOR_Format8bitRGB332";
    case OMX_COLOR_Format12bitRGB444:
      return "OMX_COLOR_Format12bitRGB444";
    case OMX_COLOR_Format16bitARGB4444:
      return "OMX_COLOR_Format16bitARGB4444";
    case OMX_COLOR_Format16bitARGB1555:
      return "OMX_COLOR_Format16bitARGB1555";
    case OMX_COLOR_Format16bitRGB565:
      return "OMX_COLOR_Format16bitRGB565";
    case OMX_COLOR_Format16bitBGR565:
      return "OMX_COLOR_Format16bitBGR565";
    case OMX_COLOR_Format18bitRGB666:
      return "OMX_COLOR_Format18bitRGB666";
    case OMX_COLOR_Format18bitARGB1665:
      return "OMX_COLOR_Format18bitARGB1665";
    case OMX_COLOR_Format19bitARGB1666:
      return "OMX_COLOR_Format19bitARGB1666";
    case OMX_COLOR_Format24bitRGB888:
      return "OMX_COLOR_Format24bitRGB888";
    case OMX_COLOR_Format24bitBGR888:
      return "OMX_COLOR_Format24bitBGR888";
    case OMX_COLOR_Format24bitARGB1887:
      return "OMX_COLOR_Format24bitARGB1887";
    case OMX_COLOR_Format25bitARGB1888:
      return "OMX_COLOR_Format25bitARGB1888";
    case OMX_COLOR_Format32bitBGRA8888:
      return "OMX_COLOR_Format32bitBGRA8888";
    case OMX_COLOR_Format32bitARGB8888:
      return "OMX_COLOR_Format32bitARGB8888";
    case OMX_COLOR_FormatYUV411Planar:
      return "OMX_COLOR_FormatYUV411Planar";
    case OMX_COLOR_FormatYUV411PackedPlanar:
      return "OMX_COLOR_FormatYUV411PackedPlanar: Planes fragmented when a frame is split in multiple buffers";
    case OMX_COLOR_FormatYUV420Planar:
      return "OMX_COLOR_FormatYUV420Planar: Planar YUV, 4:2:0 (I420)";
    case OMX_COLOR_FormatYUV420PackedPlanar:
      return "OMX_COLOR_FormatYUV420PackedPlanar: Planar YUV, 4:2:0 (I420), planes fragmented when a frame is split in multiple buffers";
    case OMX_COLOR_FormatYUV420SemiPlanar:
      return "OMX_COLOR_FormatYUV420SemiPlanar, Planar YUV, 4:2:0 (NV12), U and V planes interleaved with first U value";
    case OMX_COLOR_FormatYUV422Planar:
      return "OMX_COLOR_FormatYUV422Planar";
    case OMX_COLOR_FormatYUV422PackedPlanar:
      return "OMX_COLOR_FormatYUV422PackedPlanar: Planes fragmented when a frame is split in multiple buffers";
    case OMX_COLOR_FormatYUV422SemiPlanar:
      return "OMX_COLOR_FormatYUV422SemiPlanar";
    case OMX_COLOR_FormatYCbYCr:
      return "OMX_COLOR_FormatYCbYCr";
    case OMX_COLOR_FormatYCrYCb:
      return "OMX_COLOR_FormatYCrYCb";
    case OMX_COLOR_FormatCbYCrY:
      return "OMX_COLOR_FormatCbYCrY";
    case OMX_COLOR_FormatCrYCbY:
      return "OMX_COLOR_FormatCrYCbY";
    case OMX_COLOR_FormatYUV444Interleaved:
      return "OMX_COLOR_FormatYUV444Interleaved";
    case OMX_COLOR_FormatRawBayer8bit:
      return "OMX_COLOR_FormatRawBayer8bit";
    case OMX_COLOR_FormatRawBayer10bit:
      return "OMX_COLOR_FormatRawBayer10bit";
    case OMX_COLOR_FormatRawBayer8bitcompressed:
      return "OMX_COLOR_FormatRawBayer8bitcompressed";
    case OMX_COLOR_FormatL2:
      return "OMX_COLOR_FormatL2";
    case OMX_COLOR_FormatL4:
      return "OMX_COLOR_FormatL4";
    case OMX_COLOR_FormatL8:
      return "OMX_COLOR_FormatL8";
    case OMX_COLOR_FormatL16:
      return "OMX_COLOR_FormatL16";
    case OMX_COLOR_FormatL24:
      return "OMX_COLOR_FormatL24";
    case OMX_COLOR_FormatL32:
      return "OMX_COLOR_FormatL32";
    case OMX_COLOR_FormatYUV420PackedSemiPlanar:
      return "OMX_COLOR_FormatYUV420PackedSemiPlanar: Planar YUV, 4:2:0 (NV12), planes fragmented when a frame is split in multiple buffers, U and V planes interleaved with first U value";
    case OMX_COLOR_FormatYUV422PackedSemiPlanar:
      return "OMX_COLOR_FormatYUV422PackedSemiPlanar: Planes fragmented when a frame is split in multiple buffers";
    case OMX_COLOR_Format18BitBGR666:
      return "OMX_COLOR_Format18BitBGR666";
    case OMX_COLOR_Format24BitARGB6666:
      return "OMX_COLOR_Format24BitARGB6666";
    case OMX_COLOR_Format24BitABGR6666:
      return "OMX_COLOR_Format24BitABGR6666";
    case OMX_COLOR_Format32bitABGR8888:
      return "OMX_COLOR_Format32bitABGR8888";
    case OMX_COLOR_Format8bitPalette:
      return "OMX_COLOR_Format8bitPalette";
    case OMX_COLOR_FormatYUVUV128:
      return "OMX_COLOR_FormatYUVUV128";
    case OMX_COLOR_FormatRawBayer12bit:
      return "OMX_COLOR_FormatRawBayer12bit";
    case OMX_COLOR_FormatBRCMEGL:
      return "OMX_COLOR_FormatBRCMEGL";
    case OMX_COLOR_FormatBRCMOpaque:
      return "OMX_COLOR_FormatBRCMOpaque";
    case OMX_COLOR_FormatYVU420PackedPlanar:
      return "OMX_COLOR_FormatYVU420PackedPlanar";
    case OMX_COLOR_FormatYVU420PackedSemiPlanar:
      return "OMX_COLOR_FormatYVU420PackedSemiPlanar";
    default:
      f = (char*) calloc(23, sizeof(char));
      if (f == NULL) {
        die("Failed to allocate memory");
      }
      snprintf((char*) f, 23 * sizeof(char) - 1, "format type 0x%08x", c);
      return f;
  }
}

void OpenMAX::dump_portdef(OMX_PARAM_PORTDEFINITIONTYPE* portdef) {
  say("Port %d is %s, %s, buffers wants:%d needs:%d, size:%d, pop:%d, aligned:%d",
      portdef->nPortIndex, (portdef->eDir == OMX_DirInput ? "input" : "output"),
      (portdef->bEnabled == OMX_TRUE ? "enabled" : "disabled"),
      portdef->nBufferCountActual, portdef->nBufferCountMin,
      portdef->nBufferSize, portdef->bPopulated, portdef->nBufferAlignment);

  OMX_VIDEO_PORTDEFINITIONTYPE *viddef = &portdef->format.video;
//	OMX_IMAGE_PORTDEFINITIONTYPE *imgdef = &portdef->format.image;
  switch (portdef->eDomain) {
    case OMX_PortDomainVideo:
      say("Video type:\n"
          "\tWidth:\t\t%d\n"
          "\tHeight:\t\t%d\n"
          "\tStride:\t\t%d\n"
          "\tSliceHeight:\t%d\n"
          "\tBitrate:\t%d\n"
          "\tFramerate:\t%.02f\n"
          "\tError hiding:\t%s\n"
          "\tCodec:\t\t%s\n"
          "\tColor:\t\t%s\n",
          viddef->nFrameWidth, viddef->nFrameHeight, viddef->nStride,
          viddef->nSliceHeight, viddef->nBitrate,
          ((float) viddef->xFramerate / (float) 65536),
          (viddef->bFlagErrorConcealment == OMX_TRUE ? "yes" : "no"),
          dump_compression_format(viddef->eCompressionFormat),
          dump_color_format(viddef->eColorFormat));
      break;
    case OMX_PortDomainImage:
//		say("Image type:\n"
//				"\tWidth:\t\t%d\n"
//				"\tHeight:\t\t%d\n"
//				"\tStride:\t\t%d\n"
//				"\tSliceHeight:\t%d\n"
//				"\tError hiding:\t%s\n"
//				"\tCodec:\t\t%s\n"
//				"\tColor:\t\t%s\n", imgdef->nFrameWidth, imgdef->nFrameHeight,
//				imgdef->nStride, imgdef->nSliceHeight,
//				(imgdef->bFlagErrorConcealment == OMX_TRUE ? "yes" : "no"),
//				dump_compression_format(imgdef->eCompressionFormat),
//				dump_color_format(imgdef->eColorFormat));
      break;
    default:
      break;
  }
}

void OpenMAX::dump_port(OMX_HANDLETYPE hComponent, OMX_U32 nPortIndex,
                        OMX_BOOL dumpformats) {
  OMX_ERRORTYPE r;
  OMX_PARAM_PORTDEFINITIONTYPE portdef;
  OMX_INIT_STRUCTURE(portdef);
  portdef.nPortIndex = nPortIndex;
  if ((r = OMX_GetParameter(hComponent, OMX_IndexParamPortDefinition, &portdef))
      != OMX_ErrorNone) {
    omx_die(r, "Failed to get port definition for port %d", nPortIndex);
  }
  dump_portdef(&portdef);
  if (dumpformats) {
    OMX_VIDEO_PARAM_PORTFORMATTYPE portformat;
    OMX_INIT_STRUCTURE(portformat);
    portformat.nPortIndex = nPortIndex;
    portformat.nIndex = 0;
    r = OMX_ErrorNone;
    say("Port %d supports these video formats:", nPortIndex);
    while (r == OMX_ErrorNone) {
      if ((r = OMX_GetParameter(hComponent, OMX_IndexParamVideoPortFormat,
                                &portformat)) == OMX_ErrorNone) {
        say("\t%s, compression: %s", dump_color_format(portformat.eColorFormat),
            dump_compression_format(portformat.eCompressionFormat));
        portformat.nIndex++;
      }
    }
  }
}

// Some busy loops to verify we're running in order
void OpenMAX::block_until_state_changed(OMX_HANDLETYPE hComponent,
                                        OMX_STATETYPE wanted_eState) {
  OMX_STATETYPE eState;
  int i = 0;
  while (i++ == 0 || eState != wanted_eState) {
    OMX_GetState(hComponent, &eState);
    if (eState != wanted_eState) {
      usleep(10000);
    }
  }
}

void OpenMAX::block_until_port_changed(OMX_HANDLETYPE hComponent,
                                       OMX_U32 nPortIndex, OMX_BOOL bEnabled) {
  OMX_ERRORTYPE r;
  OMX_PARAM_PORTDEFINITIONTYPE portdef;
  OMX_INIT_STRUCTURE(portdef);
  portdef.nPortIndex = nPortIndex;
  OMX_U32 i = 0;
  while (i++ == 0 || portdef.bEnabled != bEnabled) {
    if ((r = OMX_GetParameter(hComponent, OMX_IndexParamPortDefinition,
                              &portdef)) != OMX_ErrorNone) {
      omx_die(r, "Failed to get port definition");
    }
    if (portdef.bEnabled != bEnabled) {
      usleep(10000);
    }
  }
}

void OpenMAX::block_until_flushed(appctx *ctx) {
  int quit;
  while (!quit) {
    vcos_semaphore_wait(&ctx->handler_lock);
    if (ctx->flushed) {
      ctx->flushed = 0;
      quit = 1;
    }
    vcos_semaphore_post(&ctx->handler_lock);
    if (!quit) {
      usleep(10000);
    }
  }
}

void OpenMAX::init_component_handle(const char *name,
                                    OMX_HANDLETYPE* hComponent,
                                    OMX_PTR pAppData,
                                    OMX_CALLBACKTYPE* callbacks) {
  OMX_ERRORTYPE r;
  char fullname[32];

  // Get handle
  memset(fullname, 0, sizeof(fullname));
  strcat(fullname, "OMX.broadcom.");
  strncat(fullname, name, strlen(fullname) - 1);
  say("Initializing component %s", fullname);
  if ((r = OMX_GetHandle(hComponent, fullname, pAppData, callbacks))
      != OMX_ErrorNone) {
    omx_die(r, "Failed to get handle for component %s", fullname);
  }

  // Disable ports
  OMX_INDEXTYPE types[] = { OMX_IndexParamAudioInit, OMX_IndexParamVideoInit,
      OMX_IndexParamImageInit, OMX_IndexParamOtherInit };
  OMX_PORT_PARAM_TYPE ports;
  OMX_INIT_STRUCTURE(ports);
  OMX_GetParameter(*hComponent, OMX_IndexParamVideoInit, &ports);

  int i;
  for (i = 0; i < 4; i++) {
    if (OMX_GetParameter(*hComponent, types[i], &ports) == OMX_ErrorNone) {
      OMX_U32 nPortIndex;
      for (nPortIndex = ports.nStartPortNumber;
          nPortIndex < ports.nStartPortNumber + ports.nPorts; nPortIndex++) {
        say("Disabling port %d of component %s", nPortIndex, fullname);
        if ((r = OMX_SendCommand(*hComponent, OMX_CommandPortDisable,
                                 nPortIndex, NULL)) != OMX_ErrorNone) {
          omx_die(r, "Failed to disable port %d of component %s", nPortIndex,
                  fullname);
        }
        block_until_port_changed(*hComponent, nPortIndex, OMX_FALSE);
      }
    }
  }
}
//
//// Global signal handler for trapping SIGINT, SIGTERM, and SIGQUIT
//static void OpenMAX::signal_handler(int signal) {
//	want_quit = 1;
//}

// OMX calls this handler for all the events it emits
OMX_ERRORTYPE OpenMAX::event_handler(OMX_HANDLETYPE hComponent,
                                     OMX_PTR pAppData, OMX_EVENTTYPE eEvent,
                                     OMX_U32 nData1, OMX_U32 nData2,
                                     OMX_PTR pEventData) {

  dump_event(hComponent, eEvent, nData1, nData2);

  appctx *ctx = (appctx *) pAppData;

  switch (eEvent) {
    case OMX_EventCmdComplete:
      vcos_semaphore_wait(&ctx->handler_lock);
      if (nData1 == OMX_CommandFlush) {
        ctx->flushed = 1;
      }
      vcos_semaphore_post(&ctx->handler_lock);
      break;
    case OMX_EventParamOrConfigChanged:
      vcos_semaphore_wait(&ctx->handler_lock);
      if (nData2 == OMX_IndexParamCameraDeviceNumber) {
        ctx->camera_ready = 1;
      }
      vcos_semaphore_post(&ctx->handler_lock);
      break;
    case OMX_EventBufferFlag:
      vcos_semaphore_wait(&ctx->handler_lock);
      ctx->buff_eos = true;
      vcos_semaphore_post(&ctx->handler_lock);
      break;
    case OMX_EventError:
//		omx_die(nData1, "error event received");
      break;
    default:
      break;
  }

  return OMX_ErrorNone;
}

// Called by OMX when the encoder component has filled
// the output buffer with H.264 encoded video data
OMX_ERRORTYPE OpenMAX::fill_output_buffer_done_handler(
    OMX_HANDLETYPE hComponent, OMX_PTR pAppData,
    OMX_BUFFERHEADERTYPE* pBuffer) {
  appctx *ctx = ((appctx*) pAppData);
  vcos_semaphore_wait(&ctx->handler_lock);
  // The main loop can now flush the buffer to output file
  ctx->encoder_output_buffer_available = 1;
  vcos_semaphore_post(&ctx->handler_lock);
  return OMX_ErrorNone;
}

OMX_ERRORTYPE OpenMAX::fill_output_Camerabuffer_done_handler(
    OMX_HANDLETYPE hComponent, OMX_PTR pAppData,
    OMX_BUFFERHEADERTYPE* pBuffer) {
  appctx *ctx = ((appctx*) pAppData);
  vcos_semaphore_wait(&ctx->handler_lock);
  // The main loop can now flush the buffer to output file
  ctx->camera_output_buffer_available = 1;
  vcos_semaphore_post(&ctx->handler_lock);
  return OMX_ErrorNone;
}

void OpenMAX::ResetConfigure(int _width, int _height, int _fps,
                             int _portNumber) {
  SwitchBufferFlush();
  DisablePort();
  FreeBuffer();
  omx_width = _width;
  omx_height = _height;
  omx_fps = _fps;

  OMX_INIT_STRUCTURE(camera_portdef);
  camera_portdef.nPortIndex = 70;
  if ((r = OMX_GetParameter(ctx.camera, OMX_IndexParamPortDefinition,
                            &camera_portdef)) != OMX_ErrorNone) {
    omx_die(r,
            "Failed to get port definition for camera preview output port 70");
  }
  camera_portdef.format.video.nFrameWidth = omx_width;
  camera_portdef.format.video.nFrameHeight = omx_height;
  camera_portdef.format.video.xFramerate = omx_fps << 16;
  // Stolen from gstomxvideodec.c of gst-omx
  camera_portdef.format.video.nStride = (camera_portdef.format.video.nFrameWidth
      + camera_portdef.nBufferAlignment - 1)
      & (~(camera_portdef.nBufferAlignment - 1));
  camera_portdef.format.video.eColorFormat = OMX_COLOR_FormatYUV420PackedPlanar;
  if ((r = OMX_SetParameter(ctx.camera, OMX_IndexParamPortDefinition,
                            &camera_portdef)) != OMX_ErrorNone) {
    omx_die(r,
            "Failed to set port definition for camera preview output port 70");
  }
  // Configure video format emitted by camera video output port
  // Use configuration from camera preview output as basis for
  // camera video output configuration

  camera_portdef.nPortIndex = 71;
  if ((r = OMX_SetParameter(ctx.camera, OMX_IndexParamPortDefinition,
                            &camera_portdef)) != OMX_ErrorNone) {
    omx_die(r, "Failed to set port definition for camera video output port 71");
  }

  OMX_INIT_STRUCTURE(camera_portdef);
  camera_portdef.nPortIndex = 72;
  if ((r = OMX_GetParameter(ctx.camera, OMX_IndexParamPortDefinition,
                            &camera_portdef)) != OMX_ErrorNone) {
    omx_die(r, "Failed to get port definition for camera image output port 72");
  }
  camera_portdef.format.image.nFrameWidth = omx_width;
  camera_portdef.format.image.nFrameHeight = omx_height;
  camera_portdef.format.image.nStride = (camera_portdef.format.image.nFrameWidth
      + camera_portdef.nBufferAlignment - 1)
      & (~(camera_portdef.nBufferAlignment - 1));
  if ((r = OMX_SetParameter(ctx.camera, OMX_IndexParamPortDefinition,
                            &camera_portdef)) != OMX_ErrorNone) {
    omx_die(r, "Failed to set port definition for camera image output port 72");
  }
  // Configure frame rate
  OMX_INIT_STRUCTURE(framerate);
  framerate.nPortIndex = 70;
  framerate.xEncodeFramerate = camera_portdef.format.video.xFramerate;
  if ((r = OMX_SetConfig(ctx.camera, OMX_IndexConfigVideoFramerate, &framerate))
      != OMX_ErrorNone) {
    omx_die(
        r,
        "Failed to set framerate configuration for camera preview output port 70");
  }
  framerate.nPortIndex = 71;
  if ((r = OMX_SetConfig(ctx.camera, OMX_IndexConfigVideoFramerate, &framerate))
      != OMX_ErrorNone) {
    omx_die(
        r,
        "Failed to set framerate configuration for camera video output port 71");
  }

  say("Configuring encoder...");
  say("Default port definition for encoder input port 200");
  dump_port(ctx.encoder, 200, OMX_TRUE);
  say("Default port definition for encoder output port 201");
  dump_port(ctx.encoder, 201, OMX_TRUE);

  // Encoder input port definition is done automatically upon tunneling
  // Configure video format emitted by encoder output port
  OMX_INIT_STRUCTURE(encoder_portdef);
  encoder_portdef.nPortIndex = 201;
  if ((r = OMX_GetParameter(ctx.encoder, OMX_IndexParamPortDefinition,
                            &encoder_portdef)) != OMX_ErrorNone) {
    omx_die(r, "Failed to get port definition for encoder output port 201");
  }
  // Copy some of the encoder output port configuration
  // from camera output port

  camera_portdef.nPortIndex = 70;
  if ((r = OMX_GetParameter(ctx.camera, OMX_IndexParamPortDefinition,
                            &camera_portdef)) != OMX_ErrorNone) {
    omx_die(r,
            "Failed to get port definition for camera preview output port 70");
  }
  encoder_portdef.format.video.nFrameWidth = omx_width;
  encoder_portdef.format.video.nFrameHeight = omx_height;
  encoder_portdef.format.video.xFramerate = omx_fps << 16;
  encoder_portdef.format.video.nStride = camera_portdef.format.video.nStride;

  if ((r = OMX_SetParameter(ctx.encoder, OMX_IndexParamPortDefinition,
                            &encoder_portdef)) != OMX_ErrorNone) {
    omx_die(r, "Failed to set port definition for encoder output port 201");
  }
  // Ensure camera is ready
  while (!ctx.camera_ready) {
    usleep(10000);
  }

  SwitchIdle();
  EnablePort();
  AllocateBuffer();
  SwitchExecuting();
  SwitchCapturePort(_portNumber, OMX_TRUE);

}

void OpenMAX::YUV_lookup_table() {
  int i, j, k;
  double i_value;
  for (i = 255; i >= 0; i--) {
    YY[i] = (1.164 * (i - 16.0));
    BU[i] = (2.018 * (i - 128.0));
    GV[i] = (0.831 * (i - 128.0));
    GU[i] = (0.391 * (i - 128.0));
    RV[i] = (1.596 * (i - 128.0));
  }

  for (i = 255; i >= 0; i--) {
    for (j = 255; j >= 0; j--) {
      i_value = YY[i] + BU[j];
      if (i_value > 255)
        i_value = 255;
      else if (i_value < 0)
        i_value = 0;
      YUV_B[i][j] = (int) i_value;

      i_value = YY[i] + RV[j];
      if (i_value > 255)
        i_value = 255;
      else if (i_value < 0)
        i_value = 0;
      YUV_R[i][j] = (int) i_value;
      for (k = 0; k < 256; k++) {
        i_value = YY[i] - (GU[j] + GV[k]);
        if (i_value > 255)
          i_value = 255;
        else if (i_value < 0)
          i_value = 0;
        YUV_G[i][j][k] = (int) i_value;
      }
    }
  }
}

// YUV 영상을 RGB 영상으로 바꾸는 함수
void OpenMAX::yuv420_to_rgb(unsigned char *in, unsigned char *out, int w,
                            int h) {
  int x, y;
  double imgsize = w * h;
  int w3 = w * 3;
  double uvsize = imgsize / 4.0;

  unsigned char *pY = in;
  unsigned char *pV = in + (int) imgsize;
  unsigned char *pU = in + (int) imgsize + (int) uvsize;

  int y00, y01, y10, y11;
  int u, v;
  unsigned char *p;

  // 일반적인 경우 아래의 코드 사용함.
  for (y = 0; y <= h - 2; y += 2) {
    for (x = 0; x <= w - 2; x += 2) {
      p = out + w3 * y + x * 3;
      u = *pU;
      v = *pV;

      y00 = *pY;
      y01 = *(pY + 1);
      y10 = *(pY + w);
      y11 = *(pY + w + 1);

      *(p) = YUV_B[y00][u];
      *(p + 1) = YUV_G[y00][u][v];
      *(p + 2) = YUV_R[y00][v];

      *(p + 3) = YUV_B[y01][u];
      *(p + 3 + 1) = YUV_G[y01][u][v];
      *(p + 3 + 2) = YUV_R[y01][v];

      *(p + w3) = YUV_B[y10][u];
      *(p + w3 + 1) = YUV_G[y10][u][v];
      *(p + w3 + 2) = YUV_R[y10][v];

      *(p + w3 + 3) = YUV_B[y11][u];
      *(p + w3 + 3 + 1) = YUV_G[y11][u][v];
      *(p + w3 + 3 + 2) = YUV_R[y11][v];

      pU++;
      pV++;
      pY = pY + 2;
    }
    pY = pY + w;
  }
  cout << "convert Complete" << endl;
}

