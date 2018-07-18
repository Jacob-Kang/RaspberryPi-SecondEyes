/*
 * EventManager.h
 *
 *  Created on: 2015. 1. 23.
 *      Author:  Chulho Kang
 */

#ifndef MODULE_EVENT_EVENTMANAGER_H_
#define MODULE_EVENT_EVENTMANAGER_H_

#include "TaskManager.h"
#include "CameraManager.h"
#include "ClientSocket.h"
#include "OpenMAX.h"
#include <string.h>
#include "SEcommon.h"

class EventManager : public TaskManager {
 public:
  static EventManager* get_instance();
  EventManager();
  virtual ~EventManager();
  virtual void Run();
  void MakeFileName();
  int BackgroundSubtraction(void);
  int Subtraction(SEImage img1, SEImage img2, SEImage dst);

 protected:
  virtual ReceiveQueue* CreateMessageQueue() {
    _task = EVENTDETECTMANAGER;
    return new ReceiveQueue();
  }

 private:
  CameraManager* camera;
  ClientSocket* client;
  OpenMAX* omx;
  Message receiveMessage;
  char fileName[19];

  // background Subtraction
  int threshold1;
  int threshold2;

  SEImage image;		//영상을 받은 이미지 변수
  SEImage temp;
  SEImage dest;

  OMX_U8* rgb_frame_buffer;
  SEImage cap_frame;
  static EventManager* m_EventManager;
};

#endif /* MODULE_EVENT_EVENTMANAGER_H_ */
