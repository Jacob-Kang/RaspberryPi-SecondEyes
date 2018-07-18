/*
 * CameraManager.cpp
 *
 *  Created on: 2015. 1. 29.
 *      Author:  Chulho Kang
 */

#include "../Include/CameraManager.h"

CameraManager* CameraManager::m_CameraManager = NULL;

CameraManager::CameraManager() {
  // TODO Auto-generated constructor stub
  cout << "camera manager create!" << endl;
  omx = OpenMAX::get_instance();
  omx->InitComponent();
  omx->Load_camera_driver(0);
  omx->ConfigOpenMAX(352, 282, 10, 10000000, 0, 0, 50, 0, 0, 100);
  omx->InitOpenMAXPort();
  omx->SwitchIdle();
  omx->EnablePort();
  omx->AllocateBuffer();
  omx->SwitchExecuting();
  omx->SwitchCapturePort(72, OMX_TRUE);

}

CameraManager::~CameraManager() {
  cout << "CameraManager Destroyed" << endl;
  // TODO Auto-generated destructor stub
}

CameraManager* CameraManager::get_instance() {
  if (m_CameraManager == NULL) {
    cout << "CameraManager::Not creat yet" << endl;
    m_CameraManager = new CameraManager;
  }
  return m_CameraManager;
}

void CameraManager::CaptureFrame() {
  omx->SwitchCapturePort(72, OMX_TRUE);
  omx->OMXCaptureFrame();
  omx->SwitchCapturePort(72, OMX_FALSE);
}

void CameraManager::CaptureImage(char* fileName, int quality) {
  omx->SwitchCapturePort(72, OMX_TRUE);
  omx->OMXCaptureImage(fileName, 100);
  omx->SwitchCapturePort(72, OMX_FALSE);

}

void CameraManager::CaptureVideo(string fileName, int count) {
//	video_write = new VideoWriter;
//	videoName = fileName;
//	videoName += ".avi";
//	image = cvQueryFrame(capture);
//
////	frameSize = Size((int) vc_capture.get(CV_CAP_PROP_FRAME_WIDTH),(int) vc_capture.get(CV_CAP_PROP_FRAME_HEIGHT));
//	cv_videoWriter = cvCreateVideoWriter(videoName.c_str(), CV_FOURCC('D', 'I', 'V', 'X'), MAX_FPS, cvSize(image->width, image->height), 1);
////	video_write->open(videoName, CV_FOURCC('D', 'I', 'V', 'X'), MAX_FPS,frameSize, true);
////	if (!video_write->isOpened()) {
////		std::cout << "!!! Output video could not be opened" << std::endl;
////		return;
////	}
//	double ffps = cvGetCaptureProperty(capture,CV_CAP_PROP_FPS);
//	std::cout << "start recording" <<  endl;
//	cout << "fps = " << ffps  << endl;
//	while (count <= MAX_FPS * 15) {
//		image = cvQueryFrame(capture);
//		cvWriteFrame(cv_videoWriter, image);
////		vc_capture >> frame;
////		*video_write << frame;
//
//		count++;
//	}
//	cout << "end recoding" << endl;
//	delete video_write;
}

void CameraManager::EventMake() {
  time_t time_now;
  struct tm *tm;

  time(&time_now);
  tm = localtime(&time_now);

  sprintf(event_date, "%d", tm->tm_year + 1900 - 2000);
  if (tm->tm_mon + 1 < 10) {
    sprintf(event_date + 2, "%d", 0);
    sprintf(event_date + 3, "%d", tm->tm_mon + 1);
  } else
    sprintf(event_date + 2, "%d", tm->tm_mon + 1);
  if (tm->tm_mday < 10) {
    sprintf(event_date + 4, "%d", 0);
    sprintf(event_date + 5, "%d", tm->tm_mday);
  } else
    sprintf(event_date + 4, "%d", tm->tm_mday);

  sprintf(event_date + 6, "_");

  if (tm->tm_hour < 10) {
    sprintf(event_date + 7, "%d", 0);
    sprintf(event_date + 8, "%d", tm->tm_hour);
  } else
    sprintf(event_date + 7, "%d", tm->tm_hour);
  if (tm->tm_min < 10) {
    sprintf(event_date + 9, "%d", 0);
    sprintf(event_date + 10, "%d", tm->tm_min);
  } else
    sprintf(event_date + 9, "%d", tm->tm_min);
}

