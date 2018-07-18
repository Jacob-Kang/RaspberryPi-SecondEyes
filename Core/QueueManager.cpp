/*
 * QueueManager.cpp
 *
 *  Created on: 2015. 1. 21.
 *      Author: Chulho Kang
 */

#include "../Include/QueueManager.h"

QueueManager* QueueManager::m_QueueManager = NULL;

QueueManager::QueueManager() {
  m_QueueManager = this;
}

QueueManager::~QueueManager() {
  // TODO Auto-generated destructor stub
}

QueueManager* QueueManager::get_instance() {
  if (m_QueueManager == NULL) {
    cout << "OpenMAX::Not create yet" << endl;
    m_QueueManager = new QueueManager;
  }
  return m_QueueManager;
}

void QueueManager::RegisterReceiveMessageQueue(ReceiveQueue* p_q, int _t) {
  ReceiveQueueData temp(p_q, _t);
  queueList.push_back(temp);
}

void QueueManager::SendMessage(Message _message) {	// 보내야할 메세지들을 저장할 QueueManager 내부의 queue
  //우선순위에 따른 처리
  //순서가 낮은걸 우선적으로 처리한다.

  if (serviceQueue.front().getPriority() > _message.getPriority())
    serviceQueue.push_front(_message);
  else
    serviceQueue.push_back(_message);
}

int QueueManager::SendServiceMessage() {  // 목적지 ReceiveQueue 에 service queue  있는 메세지 저장
  if (!serviceQueue.empty()) {		// 보낼 메세지가 있다면
    Message _message = serviceQueue.front();	// 우선순위 제일 높은 애를 가져오고
    serviceQueue.pop_front();					// 비운다

    ReceiveQueue* temp = getReceiveQueue(_message.getDest());  //목적지의 큐 주소를 반환
    if (temp != 0) {
      temp->Push(_message);  // 목적지 ReceiveQueue 에 저장
//			cout << "QueueServiceManager::SendServiceQueue sendTo :" << _message.getProv()<<" receiveTo : "<< _message.getDest() << endl;
      return _message.getDest();
    } else if (temp == 0) {
      std::cerr
          << "************************************QueueManager::SendServiceMessage -> Destination error**********************************"
          << endl;
    }
  }
  return 0;
}

ReceiveQueue* QueueManager::getReceiveQueue(task_t _dest) {  //목적지의 큐 주소를 반환
  int size = queueList.size();
  for (int i = 0; i < size; i++) {
    if (queueList.at(i)._task == _dest)
      return queueList.at(i).queueList;
  }

  return 0;
}

