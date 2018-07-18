/*
 * CameraManager.h
 *
 *  Created on: 2015. 1. 29.
 *      Author:  Chulho Kang
 */

#ifndef CAMERA_CAMERAMANAGER_H_
#define CAMERA_CAMERAMANAGER_H_

#include "SEcommon.h"
#include "OpenMAX.h"
#include <cstdio>

class CameraManager {
 public:
  static CameraManager* get_instance();
  CameraManager();
  virtual ~CameraManager();
  void CaptureImage(char* fileName, int quality);
  void CaptureFrame();
  void CaptureVideo(string fileName, int count);
  void EventMake();
  char* getEvent_date() {
    return event_date;
  }
  string getVideoName() {
    return videoName;
  }

 private:
  OpenMAX* omx;
  static CameraManager* m_CameraManager;
  char event_date[12];
  string videoName;

};

#endif /* CAMERA_CAMERAMANAGER_H_ */
