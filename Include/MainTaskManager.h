/*
 * MainTaskManager.h
 *
 *  Created on: 2015. 1. 21.
 *      Author: Chulho Kang
 */

#ifndef MAINTASKMANAGER_H_
#define MAINTASKMANAGER_H_

#include "../Thread/Thread.h"
#include "Message.h"
#include "CommunicationService.h"
#include "EventDetectManager.h"
#include "StreamManager.h"
#include "QueueManager.h"
#include "QueueServiceManager.h"
#include "MotorManager.h"
class MainTaskManager : public Thread {
 public:
  MainTaskManager();
  virtual ~MainTaskManager();

  void Run();

};

#endif /* MAINTASKMANAGER_H_ */
