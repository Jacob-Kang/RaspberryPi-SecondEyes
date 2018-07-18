/*
 * QueueServiceManager.cpp
 *
 *  Created on: 2015. 1. 21.
 *      Author: Chulho Kang
 */

#include "../Include/QueueServiceManager.h"

QueueServiceManager::QueueServiceManager(QueueManager* _qm)
    : _qManager(_qm) {

}

QueueServiceManager::~QueueServiceManager() {

}

void QueueServiceManager::Run() {
  cout << "QueueServiceManager Start" << endl;
  int taskNumber;
  while (1) {
    taskNumber = _qManager->SendServiceMessage();  // 목적지에게 메세지 전달시켜주는 함수 호출
    if (taskNumber == MOTORMANAGER) {
//			cout << "QueueServiceManager::SendMessageToMotorManager" << endl;
      pthread_cond_signal(&cond_MotorManager);
    } else if (taskNumber == STREAMMANAGER) {
//			cout << "queueServiceManager::SendMessageToStreamManager" << endl;
      pthread_cond_signal(&cond_StreamManager);
    } else if (taskNumber == CONVERTINGMANAGER) {
      cout << "QueueServiceManager::SendMessageToConvertingManager" << endl;
    }
    usleep(2000);
  }
}

