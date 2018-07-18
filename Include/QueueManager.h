/*
 * QueueManager.h
 *
 *  Created on: 2015. 1. 21.
 *      Author: Chulho Kang
 *
 *		전역객체,
 *      QueueManager 클래스는 TaskManager들의 큐를 관리한다.
 *
 */

#ifndef QUEUEMANAGER_H_
#define QUEUEMANAGER_H_

#include "ReceiveQueue.h"

#include <vector>
#include <deque>

#include "SEcommon.h"
using std::deque;
using std::vector;

class ReceiveQueueData {
 public:
  ReceiveQueueData(ReceiveQueue* q, task_t t)
      : queueList(q),
        _task(t) {
  }

  ReceiveQueue* queueList;	//동적 배열로 관리..
  task_t _task;  //task번호
};

class QueueManager {
 public:
  QueueManager();
  virtual ~QueueManager();
  static QueueManager* get_instance();

  void RegisterReceiveMessageQueue(ReceiveQueue* p_q, int _t);
  void SendMessage(Message _message);	// 보내야할 메세지들을 저장할 QueueManager 내부의 queue에 저장

  int SendServiceMessage();		// 목적지 ReceiveQueue 에 service queue 엥 있는 메세지 저장
  ReceiveQueue* getReceiveQueue(task_t _dest);
  int GetServiceQueueSize() {
    return serviceQueue.size();
  }

 private:
  static QueueManager* m_QueueManager;

  deque<Message> serviceQueue;
  vector<ReceiveQueueData> queueList;  //동적 배열로 관리..
};

#endif /* QUEUEMANAGER_H_ */
