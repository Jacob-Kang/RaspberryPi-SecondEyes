/*
 * StreamManager.h
 *
 *  Created on: 2015. 1. 23.
 *      Author:  Chulho Kang
 */

#ifndef CAMERA_STREAMMANAGER_H_
#define CAMERA_STREAMMANAGER_H_

#include "SEcommon.h"
#include "TaskManager.h"
#include "ExternalProtocol.h"
#include "CameraManager.h"
#include "ClientSocket.h"
#include "MotorManager.h"
#include <string.h>
#include "OpenMAX.h"

class StreamManager : public TaskManager {
 public:
  StreamManager();
  virtual ~StreamManager();
  virtual void Run();

 protected:
  virtual ReceiveQueue* CreateMessageQueue() {
    _task = STREAMMANAGER;
    return new ReceiveQueue();
  }
 private:
  Message receiveMessage;  //queue receive message
  Message sendMessage;
  unsigned char interCMD;
  packet_stream recv_stream;
  packet_return recv_stop;

  int flag_motor;
  CameraManager* camera;
  OpenMAX* omx;
  ClientSocket* client;
  MotorManager* motorManager;

};

#endif /* CAMERA_STREAMMANAGER_H_ */
