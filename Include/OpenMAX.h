/*
 * OpenMax.h
 *
 *  Created on: 2015. 2. 5.
 *      Author:  Chulho Kang
 */

#ifndef OPENMAX_H_
#define OPENMAX_H_

#include <bcm_host.h>

#include "IL/OMX_Core.h"
#include <IL/OMX_Component.h>
#include <IL/OMX_Video.h>
#include <IL/OMX_Broadcom.h>
#include <interface/vcos/vcos_semaphore.h>
#include <interface/vmcs_host/vchost.h>
#include <time.h>
#include "SEcommon.h"
#include "QueueManager.h"

#include <cstdio>
//RTSP
#include "../RTSP/omx_dump.hh"
#include "../RTSP/piRtspServer.hh"
#include <pthread.h>
#include <interface/vcos/vcos.h>

#include "JpegManager.h"

// Hard coded parameters
#define VIDEO_WIDTH                     1920
#define VIDEO_HEIGHT                    1080
#define VIDEO_FRAMERATE                 25
#define VIDEO_BITRATE                   10000000
#define CAM_DEVICE_NUMBER               0
#define CAM_SHARPNESS                   0                       // -100 .. 100
#define CAM_CONTRAST                    0                       // -100 .. 100
#define CAM_BRIGHTNESS                  50                      // 0 .. 100
#define CAM_SATURATION                  0                       // -100 .. 100
#define CAM_EXPOSURE_VALUE_COMPENSTAION 0
#define CAM_EXPOSURE_ISO_SENSITIVITY    100
#define CAM_EXPOSURE_AUTO_SENSITIVITY   OMX_FALSE
#define CAM_FRAME_STABILISATION         OMX_TRUE
#define CAM_WHITE_BALANCE_CONTROL       OMX_WhiteBalControlAuto // OMX_WHITEBALCONTROLTYPE
#define CAM_IMAGE_FILTER                OMX_ImageFilterNoise    // OMX_IMAGEFILTERTYPE
#define CAM_FLIP_HORIZONTAL             OMX_FALSE
#define CAM_FLIP_VERTICAL               OMX_FALSE

// Dunno where this is originally stolen from...
#define OMX_INIT_STRUCTURE(a) \
    memset(&(a), 0, sizeof(a)); \
    (a).nSize = sizeof(a); \
    (a).nVersion.nVersion = OMX_VERSION; \
    (a).nVersion.s.nVersionMajor = OMX_VERSION_MAJOR; \
    (a).nVersion.s.nVersionMinor = OMX_VERSION_MINOR; \
    (a).nVersion.s.nRevision = OMX_VERSION_REVISION; \
    (a).nVersion.s.nStep = OMX_VERSION_STEP\

// Our application context passed around
// the main routine and callback handlers
typedef struct {
  OMX_HANDLETYPE camera;
  OMX_BUFFERHEADERTYPE *camera_ppBuffer_in;
  OMX_BUFFERHEADERTYPE *camera_ppBuffer_out;
  int camera_output_buffer_available;
  int camera_ready;
  OMX_HANDLETYPE encoder;
  OMX_HANDLETYPE imge_encoder;
  OMX_BUFFERHEADERTYPE *image_pp;
  OMX_BUFFERHEADERTYPE *encoder_ppBuffer_out;
  int encoder_output_buffer_available;
  OMX_HANDLETYPE null_sink;
  int flushed;
  bool buff_eos;
  FILE *fd_out;
  VCOS_SEMAPHORE_T handler_lock;
} appctx;

typedef struct {
  int width;
  int height;
  size_t size;
  int buf_stride;
  int buf_slice_height;
  int buf_extra_padding;
  int p_offset[3];
  int p_stride[3];
} i420_frame_info;

#define OMX_INIT_STRUCTURE(x) \
memset (&(x), 0, sizeof (x)); \
(x).nSize = sizeof (x); \
(x).nVersion.nVersion = OMX_VERSION; \
(x).nVersion.s.nVersionMajor = OMX_VERSION_MAJOR; \
(x).nVersion.s.nVersionMinor = OMX_VERSION_MINOR; \
(x).nVersion.s.nRevision = OMX_VERSION_REVISION; \
(x).nVersion.s.nStep = OMX_VERSION_STEP

#define VIDEO_FRAMERATE 25
#define VIDEO_BITRATE 3000000
#define VIDEO_IDR_PERIOD 5 //Disabled
#define VIDEO_SEI OMX_FALSE
#define VIDEO_EEDE OMX_FALSE
#define VIDEO_EEDE_LOSS_RATE 0
#define VIDEO_QP OMX_FALSE
#define VIDEO_QP_I 0 //1 .. 51, 0 means off
#define VIDEO_QP_P 0 //1 .. 51, 0 means off
#define VIDEO_PROFILE OMX_VIDEO_AVCProfileBaseline //OMX_VIDEO_AVCProfileHigh
#define VIDEO_INLINE_HEADERS OMX_FALSE

//Some settings doesn't work well
#define CAM_WIDTH 1280
#define CAM_HEIGHT 720
#define CAM_SHARPNESS 0 //-100 .. 100
#define CAM_CONTRAST 0 //-100 .. 100
#define CAM_BRIGHTNESS 50 //0 .. 100
#define CAM_SATURATION 0 //-100 .. 100
#define CAM_SHUTTER_SPEED_AUTO OMX_TRUE
#define CAM_SHUTTER_SPEED 1.0/8.0
#define CAM_ISO_AUTO OMX_TRUE
#define CAM_ISO 100 //100 .. 800
#define CAM_EXPOSURE OMX_ExposureControlAuto
#define CAM_EXPOSURE_COMPENSATION 0 //-24 .. 24
#define CAM_MIRROR OMX_MirrorNone
#define CAM_ROTATION 0 //0 90 180 270
#define CAM_COLOR_ENABLE OMX_FALSE
#define CAM_COLOR_U 128 //0 .. 255
#define CAM_COLOR_V 128 //0 .. 255
#define CAM_NOISE_REDUCTION OMX_TRUE
#define CAM_FRAME_STABILIZATION OMX_FALSE
#define CAM_METERING OMX_MeteringModeAverage
#define CAM_WHITE_BALANCE OMX_WhiteBalControlAuto
// The gains are used if the white balance is set to off
#define CAM_WHITE_BALANCE_RED_GAIN 1000 //0 ..
#define CAM_WHITE_BALANCE_BLUE_GAIN 1000 //0 ..
#define CAM_IMAGE_FILTER OMX_ImageFilterNone
#define CAM_ROI_TOP 0 //0 .. 100
#define CAM_ROI_LEFT 0 //0 .. 100
#define CAM_ROI_WIDTH 100 //0 .. 100
#define CAM_ROI_HEIGHT 100 //0 .. 100
#define CAM_DRC OMX_DynRangeExpOff

class OpenMAX {
 public:
  OpenMAX();
  static OpenMAX* get_instance();
  void InitComponent();
  void Load_camera_driver(int num_device);
  virtual ~OpenMAX();
  void ConfigOpenMAX(int _video_width, int _video_height, int _video_fps,
                     int _video_bitrate, int _cam_sharpness, int _cam_contrast,
                     int _cam_britness, int _cam_saturation,
                     int _cam_exposure_value_compensation,
                     int _cam_exposure_iso_sensitivity);
  void InitOpenMAXPort();
  void SwitchIdle();
  void EnablePort();
  void AllocateBuffer();
  void SwitchExecuting();
  void SwitchCapturePort(int _capturePort, OMX_BOOL _portSet);
  SEImage* OMXCaptureFrame();
  void OMXCaptureImage(char* filename, int quality);
  void StartStreaming();
  void VideoEncode(const char* filename, int record_time);

  void SwitchBufferFlush();
  void DisablePort();
  void FreeBuffer();
  void SwitchStateLoaded();

  void ResetConfigure(int _width, int _height, int _fps, int _portNumber);
  void DisableComponent();

  void SignalEnd() {
    is_end = 1;
  }
  static void get_i420_frame_info(int width, int height, int buf_stride,
                                  int buf_slice_height, i420_frame_info *info);
  void set_jpeg_settings();
  static void dump_frame_info(const char *message, const i420_frame_info *info);

  void YUV_lookup_table();
  void yuv420_to_rgb(unsigned char *in, unsigned char *out, int w, int h);

  void CloseComponet();
  static void say(const char* message, ...);
  static void die(const char* message, ...);
  static void omx_die(OMX_ERRORTYPE error, const char* message, ...) {
    va_list args;
    char str[1024];
    const char *e;
    memset(str, 0, sizeof(str));
    va_start(args, message);
    vsnprintf(str, sizeof(str), message, args);
    va_end(args);
    switch (error) {
      case OMX_ErrorNone:
        e = "no error";
        break;
      case OMX_ErrorBadParameter:
        e = "bad parameter";
        break;
      case OMX_ErrorIncorrectStateOperation:
        e = "invalid state while trying to perform command";
        break;
      case OMX_ErrorIncorrectStateTransition:
        e = "unallowed state transition";
        break;
      case OMX_ErrorInsufficientResources:
        e = "insufficient resource";
        break;
      case OMX_ErrorBadPortIndex:
        e = "bad port index, i.e. incorrect port";
        break;
      case OMX_ErrorHardware:
        e = "hardware error";
        break;
        /* That's all I've encountered during hacking so let's not bother with the rest... */
      default:
        e = "(no description)";
    }
    die("OMX error: %s: 0x%08x %s", str, error, e);
  }

  static void dump_event(OMX_HANDLETYPE hComponent, OMX_EVENTTYPE eEvent,
                         OMX_U32 nData1, OMX_U32 nData2);
  static const char* dump_compression_format(OMX_VIDEO_CODINGTYPE c);
  static const char* dump_color_format(OMX_COLOR_FORMATTYPE c);
  static void dump_portdef(OMX_PARAM_PORTDEFINITIONTYPE* portdef);
  static void dump_port(OMX_HANDLETYPE hComponent, OMX_U32 nPortIndex,
                        OMX_BOOL dumpformats);

  // Some busy loops to verify we're running in order
  static void block_until_state_changed(OMX_HANDLETYPE hComponent,
                                        OMX_STATETYPE wanted_eState);
  static void block_until_port_changed(OMX_HANDLETYPE hComponent,
                                       OMX_U32 nPortIndex, OMX_BOOL bEnabled);
  static void block_until_flushed(appctx *ctx);
  static void init_component_handle(const char *name,
                                    OMX_HANDLETYPE* hComponent,
                                    OMX_PTR pAppData,
                                    OMX_CALLBACKTYPE* callbacks);

  // Global signal handler for trapping SIGINT, SIGTERM, and SIGQUIT
  static void signal_handler(int signal);

  // OMX calls this handler for all the events it emits
  static OMX_ERRORTYPE event_handler(OMX_HANDLETYPE hComponent,
                                     OMX_PTR pAppData, OMX_EVENTTYPE eEvent,
                                     OMX_U32 nData1, OMX_U32 nData2,
                                     OMX_PTR pEventData);
  // Called by OMX when the encoder component has filled
  // the output buffer with H.264 encoded video data
  static OMX_ERRORTYPE fill_output_buffer_done_handler(
      OMX_HANDLETYPE hComponent, OMX_PTR pAppData,
      OMX_BUFFERHEADERTYPE* pBuffer);

  static OMX_ERRORTYPE Camevent_handler(OMX_HANDLETYPE hComponent,
                                        OMX_PTR pAppData, OMX_EVENTTYPE eEvent,
                                        OMX_U32 nData1, OMX_U32 nData2,
                                        OMX_PTR pEventData);
  // Called by OMX when the encoder component has filled
  // the output buffer with H.264 encoded video data
  static OMX_ERRORTYPE fill_output_Camerabuffer_done_handler(
      OMX_HANDLETYPE hComponent, OMX_PTR pAppData,
      OMX_BUFFERHEADERTYPE* pBuffer);

 private:
  static OpenMAX* m_OpenMAX;

  OMX_ERRORTYPE r;
  // Init context
  appctx ctx;
  OMX_CALLBACKTYPE callbacks;
  OMX_CALLBACKTYPE camcallbacks;
  // Encoder input port definition is done automatically upon tunneling
  // Configure video format emitted by encoder output port
  OMX_PARAM_PORTDEFINITIONTYPE encoder_portdef;
  // Request a callback to be made when OMX_IndexParamCameraDeviceNumber is
  // changed signaling that the camera device is ready for use.
  OMX_CONFIG_REQUESTCALLBACKTYPE cbtype;
  // Set device number, this triggers the callback configured just above
  OMX_PARAM_U32TYPE device;
  // Configure video format emitted by camera preview output port
  OMX_PARAM_PORTDEFINITIONTYPE camera_portdef;
  OMX_PARAM_PORTDEFINITIONTYPE port73def;
//	OMX_PARAM_PORTDEFINITIONTYPE temp2_portdef;

  // Configure frame rate
  OMX_CONFIG_FRAMERATETYPE framerate;
  // Configure sharpness
  OMX_CONFIG_SHARPNESSTYPE sharpness;
  // Configure contrast
  OMX_CONFIG_CONTRASTTYPE contrast;
  // Configure saturation
  OMX_CONFIG_SATURATIONTYPE saturation;
  // Configure brightness
  OMX_CONFIG_BRIGHTNESSTYPE brightness;
  // Configure exposure value
  OMX_CONFIG_EXPOSUREVALUETYPE exposure_value;
  // Configure exposure control
  OMX_CONFIG_EXPOSURECONTROLTYPE exposure_control;
  // Configure frame frame stabilisation
  OMX_CONFIG_FRAMESTABTYPE frame_stabilisation_control;
  // Configure frame white balance control
  OMX_CONFIG_WHITEBALCONTROLTYPE white_balance_control;
  // COnfigure frame white balance gain
  OMX_CONFIG_CUSTOMAWBGAINSTYPE white_balance_gains_st;
  // Configure image filter
  OMX_CONFIG_IMAGEFILTERTYPE image_filter;
  // Configure mirror
  OMX_MIRRORTYPE eMirror;
  OMX_CONFIG_MIRRORTYPE mirror;
  // Configure bitrate
  OMX_VIDEO_PARAM_BITRATETYPE bitrate;
  // Configure format
  OMX_VIDEO_PARAM_PORTFORMATTYPE format;
  OMX_IMAGE_PARAM_PORTFORMATTYPE imgFormat;
  //capture
  OMX_CONFIG_PORTBOOLEANTYPE OMXcapture;
  //AVC profile
  OMX_VIDEO_PARAM_AVCTYPE avc_st;
  size_t output_written;

  //RTSP streamin
  pthread_t server_thread;
  PI_MEMORY_BUFFER* rtsp_buffer;
  VCOS_SEMAPHORE_T RTSPsmp;
  int is_end;

  int omx_width;
  int omx_height;
  int omx_fps;

  OMX_U8* frame_buffer;
  OMX_U8* rgb_frame_buffer;
  SEImage cap_frame;

  // yuv send

  JpegManager jpgm;

  // yuv420 to rgb888
  double YY[256], BU[256], GV[256], GU[256], RV[256];
  unsigned char YUV_B[256][256];
  unsigned char YUV_R[256][256];
  unsigned char YUV_G[256][256][256];
};
#endif
