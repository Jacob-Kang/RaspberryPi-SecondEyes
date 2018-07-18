/*
 * ReceiveQueue.cpp
 *
 *  Created on: 2015. 1. 21.
 *      Author: Chulho Kang
 */

#include "../Include/ReceiveQueue.h"

ReceiveQueue::ReceiveQueue() {
  // TODO Auto-generated constructor stub

}

ReceiveQueue::~ReceiveQueue() {
  // TODO Auto-generated destructor stub

}

bool ReceiveQueue::IsEmpty() {
  return queue.empty();
}

int ReceiveQueue::Size() {
  return queue.size();
}

Message ReceiveQueue::Front() {
  Message temp = queue.front();
  queue.pop();
  return temp;
}

void ReceiveQueue::Push(Message _m) {
  queue.push(_m);
}
