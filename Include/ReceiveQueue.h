/*
 * ReceiveQueue.h
 *
 *  Created on: 2015. 1. 21.
 *      Author: Chulho Kang
 *
 *      TaskManager는 각각 개인 ReceiveQueue를 가진다.
 *      외부의 Manager들이 전달한 Message를 저장한다.
 *
 */

#ifndef RECEIVEQUEUE_H_
#define RECEIVEQUEUE_H_

#include "Message.h"
#include <queue>
using std::queue;

class QueueManager;
class ReceiveQueue {
 public:
  friend class QueueManager;	// QueueManager가 전달할 Manager의 RecieveQueue 에 MSG 를 넣기 위해

  ReceiveQueue();
  virtual ~ReceiveQueue();

  bool IsEmpty();
  int Size();
  virtual Message Front();

 private:
  void Push(Message _m);
  std::queue<Message> queue;
};

#endif /* RECEIVEQUEUE_H_ */
