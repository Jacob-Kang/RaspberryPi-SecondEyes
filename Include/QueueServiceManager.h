/*
 * QueueServiceManager.h
 *
 *  Created on: 2015. 1. 21.
 *      Author: Chulho Kang
 *
 *
 *      QueueServiceManager는 각 TaskManager들의 송신 Message들을 전달하는 기능을 수행한다.
 *
 */

#ifndef QUEUESERVICEMANAGER_H_
#define QUEUESERVICEMANAGER_H_

#include "QueueManager.h"
#include "Message.h"
#include "../Thread/Thread.h"
#include "QueueManager.h"
#include "SEcommon.h"

class QueueServiceManager : public Thread {
 public:
  QueueServiceManager(QueueManager* _qm);
  virtual ~QueueServiceManager();
  void Run();

 private:
  QueueManager* _qManager;
};

#endif /* QUEUESERVICEMANAGER_H_ */
