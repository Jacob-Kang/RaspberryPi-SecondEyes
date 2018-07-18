/*
 * TaskManager.cpp
 *
 *  Created on: 2015. 1. 21.
 *      Author: Chulho Kang
 */

#include "../Include/TaskManager.h"

TaskManager::TaskManager()
    : _receiveQueue(0),
      _task(0) {
}
;

TaskManager::~TaskManager() {  //delete _receiveQueue;
}
;

void TaskManager::initManager(QueueManager* _qManager) {
  p_qManager = _qManager;
  _receiveQueue = CreateMessageQueue();
  p_qManager->RegisterReceiveMessageQueue(_receiveQueue, _task);  //receivequeue, task식별자 등록
}

Message TaskManager::ReceiveMessage() {
  Message temp;
  if (!ReceiveQueueIsEmpty()) {
    temp = _receiveQueue->Front();
    return temp;
  }
  return temp;
}

void TaskManager::SendMessage(Message s_msg) {
  p_qManager->SendMessage(s_msg);
}

int TaskManager::getSize() {
  return _receiveQueue->Size();
}

bool TaskManager::IsInit() {
  return _task;
}

bool TaskManager::ReceiveQueueIsEmpty() {
  return _receiveQueue->IsEmpty();
}

